const std = @import("std");
const zarc = @import("zarc.zig");

var gpa_o: std.heap.GeneralPurposeAllocator(.{}) = std.heap.GeneralPurposeAllocator(.{}){};
const gpa = gpa_o.allocator();
const zlib = std.compress.zlib;

const PArgs = struct {
    input: []u8,
    output_dir: []u8,
};

pub fn parseArgs(args: [][]u8) PArgs {
    var input: []u8 = undefined;
    var output_dir: []u8 = @constCast(".");
    var isOutputDir = false;
    
    for (args[1..]) |arg| {
        if (!isOutputDir) {
            if (std.mem.eql(u8, arg, "-d")) {
                isOutputDir = true;
            } else {
                input = arg;
            }
        } else {
            output_dir = arg;
            isOutputDir = false;
        }
    }

    return .{
        .input = input,
        .output_dir = output_dir,
    };
}

pub fn main() !u8 {
    const args = std.process.argsAlloc(gpa) catch @panic("could not get args");
    defer std.process.argsFree(gpa, args);
    
    if (args.len < 2) {
        std.debug.print("Usage: zarcd <archive.zarc> [-d output_directory]\n", .{});
        return 1;
    }
    
    const pargs: PArgs = parseArgs(args);
    
    const cwd = std.fs.cwd();
    
    const input_file = cwd.openFile(pargs.input, .{}) catch |err| {
        std.debug.panic("[ZArc] could not open archive '{s}': {any}\n", .{pargs.input, err});
    };
    defer input_file.close();
    
    const reader = input_file.reader();
    
    var magic_buf: [4]u8 = undefined;
    _ = try reader.readAll(&magic_buf);
    
    if (!std.mem.eql(u8, &magic_buf, "ZArc")) {
        std.debug.panic("[ZArc] Invalid archive format: Magic number mismatch\n", .{});
    }
    
    const files_count = try reader.readInt(u16, .little);
    std.debug.print("[ZArc] Archive contains {d} files\n", .{files_count});
    
    var output_dir = try std.fs.cwd().openDir(pargs.output_dir, .{});
    defer output_dir.close();
    
    var i: u16 = 0;
    while (i < files_count) : (i += 1) {
        const file_name_len = try reader.readInt(u16, .little);
        const file_name_buf = try gpa.alloc(u8, file_name_len);
        defer gpa.free(file_name_buf);
        _ = try reader.readAll(file_name_buf);
        
        const file_size = try reader.readInt(u32, .little);
        const compressed_data = try gpa.alloc(u8, file_size);
        defer gpa.free(compressed_data);
        _ = try reader.readAll(compressed_data);
        
        std.debug.print("[ZArc] Extracting: {s} ({d} bytes compressed)\n", .{file_name_buf, file_size});
        
        var decompressed_data = std.ArrayList(u8).init(gpa);
        defer decompressed_data.deinit();
        
        var fbs = std.io.fixedBufferStream(compressed_data);
        var stream = zlib.decompressor(fbs.reader());
        try stream.reader().readAllArrayList(&decompressed_data, std.math.maxInt(usize));
        
        const output_file = try output_dir.createFile(file_name_buf, .{});
        defer output_file.close();
        
        _ = try output_file.writeAll(decompressed_data.items);
        std.debug.print("[ZArc] Extracted: {s} ({d} bytes)\n", .{file_name_buf, decompressed_data.items.len});
    }
    
    std.debug.print("[ZArc] All files extracted successfully to {s}\n", .{pargs.output_dir});
    return 0;
}
