#[macro_use]
extern crate vulkano;
extern crate cgmath;
extern crate gio;
extern crate gtk;
extern crate rand;
extern crate vulkano_shaders;
extern crate vulkano_win;
extern crate winit;

#[allow(dead_code)]
#[allow(unused_imports)]
use vulkano::buffer::{BufferUsage, CpuAccessibleBuffer};
use vulkano::command_buffer::{AutoCommandBufferBuilder, DynamicState};
use vulkano::device::{Device, DeviceExtensions};
use vulkano::framebuffer::{Framebuffer, FramebufferAbstract, RenderPassAbstract, Subpass};
use vulkano::image::SwapchainImage;
use vulkano::instance::debug::{DebugCallback, MessageTypes};
use vulkano::instance::{Instance, PhysicalDevice};
use vulkano::pipeline::viewport::Viewport;
use vulkano::pipeline::GraphicsPipeline;
use vulkano::swapchain;
use vulkano::swapchain::{
    AcquireError, PresentMode, SurfaceTransform, Swapchain, SwapchainCreationError,
};

use gio::prelude::*;
use gio::ApplicationFlags;
use gtk::prelude::*;

use vulkano::sync;
use vulkano::sync::{FlushError, GpuFuture};

use vulkano_win::VkSurfaceBuild;

use winit::{Event, EventsLoop, VirtualKeyCode, Window, WindowBuilder, WindowEvent};

use std::sync::Arc;
use std::sync::RwLock;

use cgmath::{Deg, Matrix4, Point3, Rad};

mod archetype;
mod camera;
mod grid;
mod node;
mod shader;
mod vertex;

use camera::*;
use node::*;

fn create_instance() -> Arc<Instance> {
    let instance = {
        let mut extensions = vulkano_win::required_extensions();
        extensions.ext_debug_report = true;
        Instance::new(
            None,
            &vulkano_win::required_extensions(),
            vec!["VK_LAYER_LUNARG_standard_validation"],
        )
        .unwrap()
    };

    let _debug_callback = DebugCallback::new(
        &instance,
        MessageTypes {
            error: true,
            warning: true,
            performance_warning: true,
            information: true,
            debug: true,
        },
        |msg| {
            println!("validation layer: {:?}", msg.description);
        },
    )
    .ok();
    instance
}

fn main() {
    let instance = create_instance();
    //Choose the first available Device
    let physical = PhysicalDevice::enumerate(&instance).next().unwrap();

    //Print some info about the device currently being used
    println!(
        "Using device: {} (type: {:?})",
        physical.name(),
        physical.ty()
    );

    let mut events_loop = EventsLoop::new();
    let surface = WindowBuilder::new()
        .build_vk_surface(&events_loop, instance.clone())
        .unwrap();
    let window = surface.window();

    let queue_family = physical
        .queue_families()
        .find(|&q| q.supports_graphics() && surface.is_supported(q).unwrap_or(false))
        .unwrap();

    let device_ext = DeviceExtensions {
        khr_swapchain: true,
        ..DeviceExtensions::none()
    };
    let (device, mut queues) = Device::new(
        physical,
        physical.supported_features(),
        &device_ext,
        [(queue_family, 0.5)].iter().cloned(),
    )
    .unwrap();

    let settings_packet = std::sync::Arc::new(std::sync::RwLock::new(SettingsPacket {
        sunlight: 1.0,
        gravity: 9.8,
        moisture: 1.0,
        nitrogen: 1.0,
        potassium: 1.0,
        phosphorus: 1.0,
    }));

    gtk_setup(settings_packet.clone());

    let queue = queues.next().unwrap();

    let (mut swapchain, images) = {
        let caps = surface.capabilities(physical).unwrap();
        let usage = caps.supported_usage_flags;
        let alpha = caps.supported_composite_alpha.iter().next().unwrap();
        let format = caps.supported_formats[0].0;
        let initial_dimensions = if let Some(dimensions) = window.get_inner_size() {
            let dimensions: (u32, u32) = dimensions.to_physical(window.get_hidpi_factor()).into();
            [dimensions.0, dimensions.1]
        } else {
            return;
        };

        Swapchain::new(
            device.clone(),
            surface.clone(),
            caps.min_image_count,
            format,
            initial_dimensions,
            1,
            usage,
            &queue,
            SurfaceTransform::Identity,
            alpha,
            PresentMode::Fifo,
            true,
            None,
        )
        .unwrap()
    };

    let mut node_buffer = NodeBuffer::new(10000);
    {
        let i1 = node_buffer.alloc().unwrap();

        let mut n1 = Node::new();

        n1.status = STATUS_ALIVE;
        n1.visible = 1;
        n1.absolutePosition = [0.0, 0.0, 0.0];
        n1.transformation = Matrix4::from_angle_z(Rad(std::f32::consts::PI)).into();
        n1.length = 0.4;

        node_buffer.set(i1, n1);
    }

    let vs = shader::vert::Shader::load(device.clone()).unwrap();
    let fs = shader::frag::Shader::load(device.clone()).unwrap();

    let render_pass = Arc::new(
        single_pass_renderpass!(
                device.clone(),
                attachments: {
                    color: {
                        load: Clear,
                        store: Store,
                        format: swapchain.format(),
                        samples: 1,
                    }
                },
                pass: {
                    color: [color],
                    depth_stencil: {}
                }
        )
        .unwrap(),
    );

    let pipeline = Arc::new(
        GraphicsPipeline::start()
            .vertex_input_single_buffer()
            .vertex_shader(vs.main_entry_point(), ())
            .line_list()
            .viewports_dynamic_scissors_irrelevant(1)
            .fragment_shader(fs.main_entry_point(), ())
            .render_pass(Subpass::from(render_pass.clone(), 0).unwrap())
            .build(device.clone())
            .unwrap(),
    );

    let mut dynamic_state = DynamicState {
        line_width: None,
        viewports: None,
        scissors: None,
    };

    let mut camera = Camera::new(Point3::new(0.0, 0.0, 1.0), 50, 50, Rad::from(Deg(90.0)));
    let mut framebuffers = window_size_dependent_setup(
        &images,
        render_pass.clone(),
        &mut dynamic_state,
        &mut camera,
    );

    let mut recreate_swapchain = false;

    let mut previous_frame_end = Box::new(sync::now(device.clone())) as Box<GpuFuture>;

    loop {
        previous_frame_end.cleanup_finished();

        node_buffer.update_all();
        let vertex_buffer = {
            let vecs = node_buffer.gen_vertex();
            CpuAccessibleBuffer::from_iter(device.clone(), BufferUsage::all(), vecs.iter().cloned())
                .unwrap()
        };

        if recreate_swapchain {
            let dimensions = if let Some(dimensions) = window.get_inner_size() {
                let dimensions: (u32, u32) =
                    dimensions.to_physical(window.get_hidpi_factor()).into();
                [dimensions.0, dimensions.1]
            } else {
                return;
            };

            let (new_swapchain, new_images) = match swapchain.recreate_with_dimension(dimensions) {
                Ok(r) => r,
                Err(SwapchainCreationError::UnsupportedDimensions) => continue,
                Err(err) => panic!("{:?}", err),
            };

            swapchain = new_swapchain;
            framebuffers = window_size_dependent_setup(
                &new_images,
                render_pass.clone(),
                &mut dynamic_state,
                &mut camera,
            );

            recreate_swapchain = false;
        }

        let (image_num, acquire_future) =
            match swapchain::acquire_next_image(swapchain.clone(), None) {
                Ok(r) => r,
                Err(AcquireError::OutOfDate) => {
                    recreate_swapchain = true;
                    continue;
                }
                Err(err) => panic!("{:?}", err),
            };

        let clear_values = vec![[0.0, 0.0, 1.0, 1.0].into()];

        let command_buffer =
            AutoCommandBufferBuilder::primary_one_time_submit(device.clone(), queue.family())
                .unwrap()
                .begin_render_pass(framebuffers[image_num].clone(), false, clear_values)
                .unwrap()
                .draw(
                    pipeline.clone(),
                    &dynamic_state,
                    vertex_buffer.clone(),
                    (),
                    shader::vert::ty::PushConstantData {
                        mvp: camera.mvp().into(),
                    },
                )
                .unwrap()
                .end_render_pass()
                .unwrap()
                .build()
                .unwrap();

        let future = previous_frame_end
            .join(acquire_future)
            .then_execute(queue.clone(), command_buffer)
            .unwrap()
            .then_swapchain_present(queue.clone(), swapchain.clone(), image_num)
            .then_signal_fence_and_flush();

        match future {
            Ok(future) => {
                previous_frame_end = Box::new(future) as Box<_>;
            }
            Err(FlushError::OutOfDate) => {
                recreate_swapchain = true;
                previous_frame_end = Box::new(sync::now(device.clone())) as Box<_>;
            }
            Err(e) => {
                println!("{:?}", e);
                previous_frame_end = Box::new(sync::now(device.clone())) as Box<_>;
            }
        }

        let mut done = false;
        events_loop.poll_events(|ev| match ev {
            Event::WindowEvent {
                event: WindowEvent::CloseRequested,
                ..
            } => done = true,
            Event::WindowEvent {
                event: WindowEvent::Resized(_),
                ..
            } => recreate_swapchain = true,
            Event::WindowEvent {
                event: WindowEvent::KeyboardInput { device_id, input },
                ..
            } => {
                let _ = device_id;
                let kc = input.virtual_keycode;
                if kc.is_some() {
                    match kc.unwrap() {
                        VirtualKeyCode::W => camera.dir_move(CameraMovementDir::Upward),
                        VirtualKeyCode::A => camera.dir_move(CameraMovementDir::Left),
                        VirtualKeyCode::S => camera.dir_move(CameraMovementDir::Downward),
                        VirtualKeyCode::D => camera.dir_move(CameraMovementDir::Right),

                        VirtualKeyCode::Up => camera.dir_rotate(CameraRotationDir::Upward),
                        VirtualKeyCode::Left => camera.dir_rotate(CameraRotationDir::Left),
                        VirtualKeyCode::Down => camera.dir_rotate(CameraRotationDir::Downward),
                        VirtualKeyCode::Right => camera.dir_rotate(CameraRotationDir::Right),
                        _ => (),
                    }
                }
            }
            _ => (),
        });
        if done {
            return;
        }
    }
}

#[derive(Debug, Clone, Copy)]
struct SettingsPacket {
    pub sunlight: f64,
    pub gravity: f64,
    pub moisture: f64,
    pub nitrogen: f64,
    pub potassium: f64,
    pub phosphorus: f64,
}

fn gtk_setup(settings_packet: Arc<RwLock<SettingsPacket>>) -> () {
    std::thread::spawn(move || {
        let application = gtk::Application::new(
            "com.github.gtk-rs.examples.basic",
            ApplicationFlags::empty(),
        )
        .expect("Initialization failed...");
        application.connect_activate(move |app| {
            let window = gtk::ApplicationWindow::new(app);

            window.set_title("GUI");
            window.set_border_width(10);
            window.set_position(gtk::WindowPosition::Center);
            window.set_default_size(350, 350);

            let sunlight_scale =
                gtk::Scale::new_with_range(gtk::Orientation::Horizontal, 0.0, 1.0, 0.01);
            let gravity_scale =
                gtk::Scale::new_with_range(gtk::Orientation::Horizontal, 0.0, 20.0, 0.1);
            let moisture_scale =
                gtk::Scale::new_with_range(gtk::Orientation::Horizontal, 0.0, 1.0, 0.01);

            sunlight_scale.set_size_request(200, 10);
            gravity_scale.set_size_request(200, 10);
            moisture_scale.set_size_request(200, 10);

            let sunlight_cloned_settings_packet = settings_packet.clone();
            sunlight_scale.connect_value_changed(move |sc| {
                let mut w = sunlight_cloned_settings_packet.write().unwrap();
                w.sunlight = sc.get_value();
            });

            let gravity_cloned_settings_packet = settings_packet.clone();
            gravity_scale.connect_value_changed(move |sc| {
                let mut w = gravity_cloned_settings_packet.write().unwrap();
                w.gravity = sc.get_value();
            });

            let moisture_cloned_settings_packet = settings_packet.clone();
            moisture_scale.connect_value_changed(move |sc| {
                let mut w = moisture_cloned_settings_packet.write().unwrap();
                w.moisture = sc.get_value();
            });

            let sunlight_label = gtk::Label::new("Sunlight");
            let gravity_label = gtk::Label::new("Gravity");
            let moisture_label = gtk::Label::new("Moisture");

            let sunlight = gtk::Box::new(gtk::Orientation::Horizontal, 1);
            let gravity = gtk::Box::new(gtk::Orientation::Horizontal, 1);
            let moisture = gtk::Box::new(gtk::Orientation::Horizontal, 1);

            sunlight.add(&sunlight_label);
            sunlight.add(&sunlight_scale);

            gravity.add(&gravity_label);
            gravity.add(&gravity_scale);

            moisture.add(&moisture_label);
            moisture.add(&moisture_scale);

            let vbox = gtk::Box::new(gtk::Orientation::Vertical, 1);

            vbox.add(&sunlight);
            vbox.add(&gravity);
            vbox.add(&moisture);
            window.add(&vbox);
            window.show_all();
        });

        application.run(&[] as &[&str]);
    });
}

fn window_size_dependent_setup(
    images: &[Arc<SwapchainImage<Window>>],
    render_pass: Arc<RenderPassAbstract + Send + Sync>,
    dynamic_state: &mut DynamicState,
    camera: &mut Camera,
) -> Vec<Arc<FramebufferAbstract + Send + Sync>> {
    let dimensions = images[0].dimensions();

    let viewport = Viewport {
        origin: [0.0, 0.0],
        dimensions: [dimensions[0] as f32, dimensions[1] as f32],
        depth_range: 0.0..1.0,
    };
    camera.setscreen(dimensions[0], dimensions[1]);
    dynamic_state.viewports = Some(vec![viewport]);

    images
        .iter()
        .map(|image| {
            Arc::new(
                Framebuffer::start(render_pass.clone())
                    .add(image.clone())
                    .unwrap()
                    .build()
                    .unwrap(),
            ) as Arc<FramebufferAbstract + Send + Sync>
        })
        .collect::<Vec<_>>()
}
