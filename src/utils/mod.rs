use cgmath::{Angle, InnerSpace, Matrix4, One, Point3, Rad, Vector3};

#[derive(Copy, Clone, Debug)]
pub struct Vertex {
    pub loc: [f32; 3],
    pub color: [f32; 3],
}
impl_vertex!(Vertex, loc, color);

pub enum CameraMovementDir {
    Forward,
    Backward,
    Upward,
    Downward,
    Left,
    Right,
}

pub enum CameraRotationDir {
    Upward,
    Downward,
    Left,
    Right,
}

#[derive(Copy, Clone, Debug)]
pub struct Camera {
    fovy: Rad<f32>, //Vertical field of view
    screen_x: u32,  //Horizontal size of screen
    screen_y: u32,  //Vertical size of screen

    worldup: Vector3<f32>, // The normalized vector that the camera percieves to be up

    yaw: Rad<f32>,   //Euler angle side to side motion
    pitch: Rad<f32>, //Euler angle up and down motion (roll is gon

    loc: Point3<f32>, // The camera's location in 3d space

    up: Vector3<f32>,    //Vector pointing up above the camera
    front: Vector3<f32>, // Vector pointing in front of the camera
    right: Vector3<f32>, // Vector pointing to the right of the camera

    projection: Matrix4<f32>, // Projection Matrix
    model: Matrix4<f32>,      // Model Matrix
    view: Matrix4<f32>,       // View Matrix
}

impl Camera {
    pub fn new(location: Point3<f32>, screen_x: u32, screen_y: u32, fovy: Rad<f32>) -> Camera {
        let mut cam = Camera {
            fovy: fovy,
            screen_x: screen_x,
            screen_y: screen_y,
            worldup: Vector3::new(0.0, 1.0, 0.0),
            yaw: Rad(0.0),
            pitch: Rad(0.0),
            loc: location,
            up: Vector3::new(0.0, 0.0, 0.0),
            front: Vector3::new(0.0, 0.0, 0.0),
            right: Vector3::new(0.0, 0.0, 0.0),
            projection: Matrix4::one(),
            model: Matrix4::one(),
            view: Matrix4::one(),
        };
        cam.genmodel();
        cam.genview();
        cam.genprojection();
        cam.genvectors();
        cam
    }

    pub fn mvp(&self) -> Matrix4<f32> {
        self.projection * self.view * self.model
    }

    pub fn translate(&mut self, delta: Vector3<f32>) -> () {
        self.setloc(self.loc + delta);
    }

    pub fn rotate(&mut self, dyaw: Rad<f32>, dpitch: Rad<f32>) -> () {
        self.setrot(self.yaw + dyaw, self.pitch + dpitch);
    }

    pub fn setloc(&mut self, loc: Point3<f32>) {
        self.loc = loc;
        self.genview();
    }

    pub fn setrot(&mut self, yaw: Rad<f32>, pitch: Rad<f32>) -> () {
        self.yaw = yaw;
        self.pitch = pitch;
        self.genvectors();
    }

    pub fn dir_move(&mut self, dir: CameraMovementDir) -> () {
        match dir {
            CameraMovementDir::Forward => self.translate(self.front),
            CameraMovementDir::Backward => self.translate(-self.front),
            CameraMovementDir::Right => self.translate(self.right),
            CameraMovementDir::Left => self.translate(-self.right),
            CameraMovementDir::Upward => self.translate(self.up),
            CameraMovementDir::Downward => self.translate(-self.up),
        }
    }

    pub fn dir_rotate(&mut self, dir: CameraRotationDir) -> () {
        match dir {
            CameraRotationDir::Right => self.translate(self.right),
            CameraRotationDir::Left => self.translate(-self.right),
            CameraRotationDir::Upward => self.translate(self.up),
            CameraRotationDir::Downward => self.translate(-self.up),
        }
    }

    pub fn setscreen(&mut self, screen_x: u32, screen_y: u32) -> () {
        self.screen_x = screen_x;
        self.screen_y = screen_y;
        self.genprojection();
    }

    fn genview(&mut self) -> () {
        // Look at the place in front of us
        self.view = Matrix4::look_at(self.loc, self.loc + self.front, self.up);
    }

    fn genmodel(&mut self) -> () {
        self.model = Matrix4::one()
    }

    fn genprojection(&mut self) -> () {
        self.projection = cgmath::perspective(
            self.fovy,
            self.screen_x as f32 / self.screen_y as f32,
            0.1,
            100.0,
        )
    }

    fn genvectors(&mut self) -> () {
        // Calculate the new Front vector
        self.front = Vector3::new(
            (self.yaw * self.pitch.cos()).cos(),
            (self.pitch).sin(),
            (self.yaw * self.pitch.cos()).sin(),
        )
        .normalize();

        // Also re-calculate the Right and Up vector
        // Normalize the vectors, because their length gets closer to 0
        // the more you look up or down which results in slower movement.
        self.right = self.front.cross(self.worldup).normalize();
        self.up = self.right.cross(self.front).normalize();
    }
}
