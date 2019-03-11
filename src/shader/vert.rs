vulkano_shaders::shader! {
ty: "vertex",
    src: "
#version 450
layout(location = 0) in vec3 loc;
layout(location = 1) in vec3 color;

layout(push_constant) uniform PushConstantData {
    mat4 mvp;
} pc;

layout(location = 0) out vec3 fragColor;
void main() {
    gl_Position = pc.mvp * vec4(loc, 1.0);
    fragColor = color;
}"
}
