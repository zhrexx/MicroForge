
pub const ZArcFileEntry = struct {
    file_name: []u8,
    file_size: u32, // compressed
    file_content: []u8, // compressed
};

pub const ZArcFileTable = struct {
    files: []ZArcFileEntry, 
    files_count: u16, // optimal up to 65535 files
};

pub const ZArcMeta = struct {
    magic: [4]u8,
    file_table: ZArcFileTable, 
};
