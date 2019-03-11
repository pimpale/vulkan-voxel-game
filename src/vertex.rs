#[derive(Copy, Clone, Debug)]
pub struct Vertex {
    pub loc: [f32; 3],
    pub color: [f32; 3],
}
impl_vertex!(Vertex, loc, color);
