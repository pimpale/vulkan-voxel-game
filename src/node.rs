#![allow(dead_code)]
#![allow(unused_imports)]
#![allow(unused_variables)]
#![allow(non_snake_case)]
use cgmath::{Matrix4, Rad, Transform, Vector3, Vector4};
use std::ops::Add;

use super::archetype::INVALID_ARCHETYPE_INDEX;
use super::vertex::Vertex;

pub const INVALID_INDEX: u32 = std::u32::MAX;

pub const STATUS_GARBAGE: u32 = 0; //Default For Node, signifies that the node is not instantiated
pub const STATUS_DEAD: u32 = 1; //Node was once alive, but not anymre. It is susceptible to rot
pub const STATUS_ALIVE: u32 = 2; //Node is currently alive, and could become dead
pub const STATUS_NEVER_ALIVE: u32 = 3; //Node is not alive, and cannot die

#[repr(C)]
#[derive(Debug, Copy, Clone)]
pub struct Node {
    pub leftChildIndex: u32, // Index in node buffer set to max uint32 (INVALID_INDEX) for null
    pub rightChildIndex: u32, // Index node buffer set to max unint32 (INVALID_INDEX) for null
    pub parentIndex: u32,    // Index node buffer set to max unint32 (INVALID_INDEX) for null
    pub age: u32,            // Age of plant in ticks
    pub archetype: u32,      // Index of archetype in archetype table. Invalid archetype -> Dead
    pub status: u32,         // Current status of this plant
    pub area: f32,           // The plant's area (used for photosynthesis, etc)
    pub length: f32,         // Length of component
    pub visible: u32,        // If the node is visible
    pub absolutePosition: [f32; 3], // Absolute offset if there is no parent node
    pub transformation: [[f32; 4]; 4], //Transformation from parent node
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
            free_stack: (0..size).collect(),             // Create list of all free node locations
            free_ptr: size,                              // The current pointer to the active stack
            max_size: size, // The maximum size to which the stack may grow
        }
    }

    pub fn get(&self, index: u32) -> Node {
        self.node_list[index as usize].clone()
    }

    pub fn set(&mut self, index: u32, node: Node) {
        self.node_list[index as usize] = node.clone();
    }

    /// Returns the index of a free spot in the array (user needs to mark the spot as not garbage)
    pub fn alloc(&mut self) -> Option<u32> {
        if self.free_ptr == 0 {
            println!("No Memory Left In Array");
            None
        } else {
            self.free_ptr = self.free_ptr - 1;
            Some(self.free_stack[self.free_ptr as usize])
        }
    }

    pub fn alloc_insert(&mut self, node: Node) -> () {
        let index = self.alloc().unwrap();
        self.set(index, node.clone());
    }

    /// Marks an index in the array as free to use, marks any node as garbage
    pub fn free(&mut self, index: u32) -> () {
        self.node_list[index as usize].status = STATUS_GARBAGE;
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
        for node_index in 0..self.max_size {
            let node = &self.node_list[node_index as usize];
            if node.status != STATUS_GARBAGE && node.parentIndex == INVALID_INDEX {
                vertex_list.append(&mut self.gen_node_vertex(
                    tov(node.absolutePosition),
                    Matrix4::one(),
                    node_index,
                ));
            }
        }
        vertex_list
    }

    fn gen_node_vertex(
        &self,
        source_point: Vector3<f32>,
        parent_rotation: Matrix4<f32>,
        node_index: u32,
    ) -> Vec<Vertex> {
        let mut vertex_list = Vec::new();
        let node = self.node_list[node_index as usize];
        let total_rotation = parent_rotation * tomat(node.transformation);
        let end_loc =
            source_point + total_rotation.transform_vector(Vector3::unit_y() * node.length);
        if node.visible == 1 {
            vertex_list.push(Vertex {
                loc: to3(source_point),
                color: [0.0, 1.0, 0.0],
            });
            vertex_list.push(Vertex {
                loc: to3(end_loc),
                color: [0.0, 0.0, 0.0],
            });
        };
        if node.leftChildIndex != INVALID_INDEX {
            vertex_list.append(&mut self.gen_node_vertex(
                end_loc,
                total_rotation,
                node.leftChildIndex,
            ));
        }
        if node.rightChildIndex != INVALID_INDEX {
            vertex_list.append(&mut self.gen_node_vertex(
                end_loc,
                total_rotation,
                node.rightChildIndex,
            ));
        }
        vertex_list
    }

    pub fn set_left_child(&mut self, parent: u32, child: u32) -> () {
        self.node_list[parent as usize].leftChildIndex = child;
        if child != INVALID_INDEX {
            self.node_list[child as usize].parentIndex = parent;
        }
    }

    pub fn set_right_child(&mut self, parent: u32, child: u32) -> () {
        self.node_list[parent as usize].rightChildIndex = child;
        if child != INVALID_INDEX {
            self.node_list[child as usize].parentIndex = parent;
        }
    }

    /// Divides node segment in two, allocating a new node for the upper half, with the current
    /// node as a parent. The children are transferred to the new node. All properties of the old
    /// node are maintained, except for children. The left child is the new node, and the right one
    /// is left empty
    pub fn divide(&mut self, percentbreak: f32, node_index: u32) -> u32 {
        // Allocate a spot for the new node
        let new_node_index = self.alloc().unwrap();

        //New node shares all properties with old one
        self.node_list[new_node_index as usize] = self.node_list[node_index as usize].clone();
        //Set lengths so they add up to same amount TODO ensure percentbreak is less than one
        let origlength = self.node_list[node_index as usize].length;
        self.node_list[node_index as usize].length = percentbreak * origlength;
        self.node_list[new_node_index as usize].length = (1.0 - percentbreak) * origlength;

        //Remove any transformation from the new node
        self.node_list[new_node_index as usize].transformation = cgmath::Matrix4::one().into();

        //Join the two children of the current node on as children of the new node
        self.set_left_child(
            new_node_index,
            self.node_list[node_index as usize].leftChildIndex,
        );
        self.set_right_child(
            new_node_index,
            self.node_list[node_index as usize].rightChildIndex,
        );

        // Join the new node as the left child of the current one, and emptying the right one
        self.set_left_child(node_index, new_node_index);
        self.set_right_child(node_index, INVALID_INDEX);
        //Return the index of the new node created
        new_node_index
    }

    /// Divides a branch in two by the percent specified by percent break, and inserts the
    /// specified node index at that location
    pub fn branch(
        &mut self,
        branch_parent_index: u32,
        percentbreak: f32,
        branch_child_index: u32,
    ) -> () {
        self.divide(percentbreak, branch_parent_index);
        self.set_right_child(branch_parent_index, branch_child_index);
    }

    /// Does a nodeupdate on all nodes within the buffer that are not garbage
    pub fn update_all(&mut self) {
        for i in 0..self.max_size {
            let node = self.node_list[i as usize];
            if node.status != STATUS_GARBAGE {
                let r = rand::random::<f32>();
                self.node_list[i as usize].length = node.length * 1.0001;
                if r > 0.9995 {
                    let ni = self.alloc().unwrap();
                    self.node_list[ni as usize] = {
                        let mut nnode = Node::new();
                        nnode.status = STATUS_ALIVE;
                        nnode.visible = 1;
                        nnode.length = 0.1;
                        nnode.transformation =
                            Matrix4::from_angle_z(Rad(rand::random::<f32>() - 0.5)).into();
                        nnode
                    };
                    self.branch(i, 0.5, ni);
                }
            }
        }
    }
}

fn tomat(mat: [[f32; 4]; 4]) -> Matrix4<f32> {
    Matrix4::from_cols(
        Vector4::from(mat[0]),
        Vector4::from(mat[1]),
        Vector4::from(mat[2]),
        Vector4::from(mat[3]),
    )
}

fn tov(v3: [f32; 3]) -> Vector3<f32> {
    Vector3::new(v3[0], v3[1], v3[2])
}

fn to3(v: Vector3<f32>) -> [f32; 3] {
    [v.x, v.y, v.z]
}

fn scale3(a: [f32; 3], scalar: f32) -> [f32; 3] {
    [a[0] * scalar, a[1] * scalar, a[2] * scalar]
}

fn add3(a: [f32; 3], b: [f32; 3]) -> [f32; 3] {
    [a[0] + b[0], a[1] + b[1], a[2] + b[2]]
}

impl Node {
    pub fn new() -> Node {
        Node {
            leftChildIndex: INVALID_INDEX,
            rightChildIndex: INVALID_INDEX,
            parentIndex: INVALID_INDEX,
            age: 0,
            archetype: INVALID_ARCHETYPE_INDEX,
            status: STATUS_GARBAGE,
            visible: 0,
            area: 0.0,
            length: 0.0,
            absolutePosition: [0.0, 0.0, 0.0],
            transformation: Matrix4::one().into(),
        }
    }
}
