const std = @import("std");
const zarc = @import("zarc.zig");

var gpa_o: std.heap.GeneralPurposeAllocator(.{}) = std.heap.GeneralPurposeAllocator(.{}){};
const gpa = gpa_o.allocator();
const zlib = std.compress.zlib;

fn format(comptime fmt: []const u8, args: anytype) ![]u8 {
    return std.fmt.allocPrint(gpa, fmt, args);
}

const PArgs = struct {
    output: []u8,
    files: std.ArrayList([]u8),
};

pub fn parseArgs(args: [][]u8) PArgs {
    var files = std.ArrayList([]u8).init(gpa);
    var output: []u8 = @constCast("output.zarc");
    var isOutput = false;
    for (args[1..]) |arg| {
        if (!isOutput) {
            if (std.mem.eql(u8, arg, "-o")) {
                isOutput = true;
            } else {
                files.append(arg) catch zarc.panic("could not append new input file", .{}); 
            }
        } else {
            output = arg;
            isOutput = false;
        }
    }

    return .{
        .output = output,
        .files = files,
    };
}

pub fn extractFileName(path: []u8) []u8 {
    var i: usize = path.len;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' or path[i] == '\\') {
            return path[i + 1..];
        }
    }
    return path;
}

pub fn main() !u8 {
    const args = std.process.argsAlloc(gpa) catch zarc.panic("could not get args", .{});
    defer std.process.argsFree(gpa, args);
    
    var file_entries = std.ArrayList(zarc.ZArcFileEntry).init(gpa);
    defer file_entries.deinit();
    const pargs: PArgs = parseArgs(args);
    defer pargs.files.deinit();

    const cwd = std.fs.cwd();
    var buf = std.ArrayList(u8).init(gpa);
    defer buf.deinit();

    for (pargs.files.items) |file_path| {
        const file: std.fs.File = cwd.openFile(file_path, .{}) catch |err| {
            zarc.panic("[ZArc] could not open file '{s}': {any}\n", .{file_path, err});
        };
        defer file.close();

        const file_content = file.readToEndAlloc(gpa, 1024 * 1024) catch zarc.panic("could not read file", .{});
        defer gpa.free(file_content);

        var cmp = try zlib.compressor(buf.writer(), .{});
        _ = try cmp.write(file_content);
        try cmp.finish();
        
        const compressed = try gpa.dupe(u8, buf.items);
        std.debug.print("Compressed {s} from {d} to {d}\n", .{file_path, file_content.len, compressed.len});
        const file_entry: zarc.ZArcFileEntry = .{
            .file_name = extractFileName(file_path), 
            .file_content = compressed,
            .file_size = @intCast(compressed.len)
        };

        file_entries.append(file_entry) catch zarc.panic("could not append file entry", .{});
        buf.clearAndFree();
    }
   
    const outputFile: std.fs.File = cwd.createFile(pargs.output, .{}) catch |err| {
        zarc.panic("[ZArc] could not open file '{s}': {any}\n", .{pargs.output, err});
    };
    defer outputFile.close(); 
    
    const outputWriter: std.fs.File.Writer = outputFile.writer();
    const ZArcOutput: zarc.ZArcMeta = .{
        .magic = "ZArc".*,
        .file_table = .{
            .files = file_entries.items,
            .files_count = @intCast(file_entries.items.len),
        },
    }; 

    try outputWriter.writeAll(&ZArcOutput.magic);

    try outputWriter.writeInt(u16, ZArcOutput.file_table.files_count, .little);

    for (ZArcOutput.file_table.files) |entry| {
        try outputWriter.writeInt(u16, @intCast(entry.file_name.len), .little);
        try outputWriter.writeAll(entry.file_name);
        try outputWriter.writeInt(u32, entry.file_size, .little);
        try outputWriter.writeAll(entry.file_content);
        defer gpa.free(entry.file_content);
    }

    return 0;
}
