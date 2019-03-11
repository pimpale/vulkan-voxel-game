#[repr(C)]
pub struct Node {
    pub leftChildIndex: u32,
    pub rightChildIndex: u32,
    pub parentIndex: u32,
    pub age: u32,
    pub archetype: u32,
    pub visible: u32,
    pub width: f32,
    pub dispX: f32,
    pub dispY: f32,
    pub dispZ: f32,
}
