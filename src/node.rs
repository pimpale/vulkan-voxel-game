use super::vertex::Vertex;
use super::archetype::*;

const INVALID_INDEX:u32 = std::u32::MAX;

const STATUS_GARBAGE:u32 = 0; //Default For Node, signifies that the node is not instantiated
const STATUS_DEAD:u32 = 1; //Node was once alive, but not anymre. It is susceptible to rot
const STATUS_ALIVE:u32 = 2; //Node is currently alive, and could become dead
const STATUS_NEVER_ALIVE:u32 = 3; //Node is not alive, and cannot die

#[repr(C)]
#[derive(Debug, Clone)]
pub struct Node {
    pub leftChildIndex: u32, // Index in node buffer set to max uint32 (INVALID_INDEX) for null
    pub rightChildIndex: u32, // Index node buffer set to max unint32 (INVALID_INDEX) for null
    pub parentIndex: u32, // Index node buffer set to max unint32 (INVALID_INDEX) for null
    pub age: u32, //Age of plant in ticks
    pub archetype: u32, // Index of archetype in archetype table. Invalid archetype -> Dead 
    pub status: u32, // Current status of this plant 
    pub area: f32, // The plant's area (used for photosynthesis, etc)
    pub visible: u32, // If the node is visible 
    pub displacement: [f32; 3], //Displacement from the parent node. If parent node is null, then this is offset
    pub lengthVector: [f32, 3], //This is a vector representing the length of this node
}

#[derive(Debug, Clone)]
pub struct NodeBuffer {
    node_list: Vec<Node>,
    free_stack: Vec<u32>,
    free_ptr: u32,
    max_size: u32,
}

impl NodeBuffer {
    pub fn new(size: u32) -> NodeBuffer {
        if size == 0 || size == INVALID_INDEX {
            panic!("invalid size for node buffer")
        }
        NodeBuffer {
            node_list: vec![Node::new(); size as usize], // Create list with default nodes
            free_stack: ((size - 1)..0).collect(),       // Create list of all free node locations
            free_ptr: size,                              // The current pointer to the active stack
            max_size: size,                              // The maximum size to which the stack may grow
        }
    }

    /// Returns the index of a free spot in the array
    pub fn alloc(&mut self) -> Option<u32> {
        if self.free_ptr == 0 {
            println!("No Memory Left In Array");
            None
        } else {
            self.free_ptr = self.free_ptr - 1;
            Some(self.free_stack[self.free_ptr as usize])
        }
    }

    /// Marks an index in the array as free to use
    pub fn free(&mut self, index: u32) -> () {
        if self.free_ptr == self.max_size {
            panic!("Free Stack Full (This should not happen)");
        } else {
            self.free_stack[self.free_ptr as usize] = index;
            self.free_ptr = self.free_ptr + 1;
        }
    }

    /// Rreturns maximum size that the node list could grow to.
    pub fn size(&self) -> u32 {
        self.max_size
    }

    /// Returns the current size that the node buffer is at
    pub fn current_size(&self) -> u32 {
        self.max_size - self.free_ptr
    }

    pub fn gen_vertex(&self) -> Vec<Vertex> {
        //Vector to hold all new vertexes
        let mut vertex_list = Vec::new();

        //search for root node (null parent, visible)
        for i in 0..(self.free_ptr-1){
            let node = self.node_list[i];
            if node.archetype != INVALID_ARCHETYPE_INDEX && node.parentIndex == INVALID_INDEX  {
                vertex_list.append(self.gen_node_vertex(

        }
        
        //gen node vertex for
        
    }

    pub fn gen_node_vertex(&self, source_point: [f32; 3], selected_index: u32) -> Vec<Vertex> {}
}

impl Node {
    pub fn new() -> Node {
        Node {
            leftChildIndex: INVALID_INDEX,
            rightChildIndex: INVALID_INDEX,
            parentIndex:  INVALID_INDEX,
            age: 0,
            archetype: INVALID_ARCHETYPE_INDEX,
            visible: 0,
            area: 0.0,
            displacement: [0.0, 0.0, 0.0],
        }
    }
}
