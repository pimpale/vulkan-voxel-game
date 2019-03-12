pub const INVALID_ARCHETYPE_INDEX: u32 = std::u32::MAX;

#[repr(C)]
#[derive(Debug, Clone)]
pub struct Archetype {
    pub placeholder: u8,
}

pub struct ArchetypeTable {
    table: Vec<Archetype>,
}
