const std = @import("std");
const builtin = @import("builtin");

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


pub fn panic(comptime msg: []const u8, args: anytype) noreturn {
    const stderr = std.io.getStdErr().writer();
    const thread_id = std.Thread.getCurrentId();

    _ = stderr.print(
        "thread[{d}] panic: ",
        .{ thread_id }
    ) catch {};

    _ = stderr.print(msg, args) catch {
        _ = stderr.writeAll("failed to format panic message") catch {};
    };
    
    const stacktrace_enabled = blk: {
        const allocator = std.heap.page_allocator;
        break :blk std.process.getEnvVarOwned(allocator, "STACKTRACE") catch null != null;
    };

    if (stacktrace_enabled) {
        var addr_buf: [16]usize = undefined;
        var stacktrace = std.builtin.StackTrace{
            .index = 0,
            .instruction_addresses = addr_buf[0..],
        };
        std.debug.captureStackTrace(null, &stacktrace);

        if (stacktrace.index == 0) {
            stderr.writeAll("stack trace: <empty>\n") catch {};
        } else {
            stderr.writeAll("stack trace:\n") catch {};
            std.debug.dumpStackTrace(stacktrace);
        }
    } else {
        _ = stderr.writeAll(
            "note: Set environment variable 'STACKTRACE=1' to enable stack trace\n"
        ) catch {};
    }

    std.process.exit(1);
}
