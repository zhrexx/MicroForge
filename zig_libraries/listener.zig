const std = @import("std");

pub fn EventListener(comptime EventType: type, comptime DataType: type) type {
    return struct {
        const Self = @This();
        const ListenerFn = *const fn (DataType) void;
        const ListenerMap = std.AutoHashMap(EventType, std.ArrayList(ListenerFn));
        
        allocator: std.mem.Allocator,
        listeners: ListenerMap,

        pub fn init(allocator: std.mem.Allocator) Self {
            return .{
                .allocator = allocator,
                .listeners = ListenerMap.init(allocator),
            };
        }

        pub fn deinit(self: *Self) void {
            var it = self.listeners.iterator();
            while (it.next()) |entry| {
                entry.value_ptr.deinit();
            }
            self.listeners.deinit();
        }

        pub fn addListener(self: *Self, event: EventType, listener: ListenerFn) !void {
            var listeners = try self.listeners.getOrPut(event);
            if (!listeners.found_existing) {
                listeners.value_ptr.* = std.ArrayList(ListenerFn).init(self.allocator);
            }
            try listeners.value_ptr.append(listener);
        }

        pub fn removeListener(self: *Self, event: EventType, listener: ListenerFn) void {
            if (self.listeners.getEntry(event)) |entry| {
                var listeners = entry.value_ptr;
                for (listeners.items, 0..) |item, i| {
                    if (item == listener) {
                        _ = listeners.swapRemove(i);
                        break;
                    }
                }
                if (listeners.items.len == 0) {
                    listeners.deinit();
                    _ = self.listeners.remove(event);
                }
            }
        }

        pub fn removeAllListeners(self: *Self, event: EventType) void {
            if (self.listeners.remove(event)) |entry| {
                entry.value.deinit();
            }
        }

        pub fn emit(self: *Self, event: EventType, data: DataType) void {
            if (self.listeners.get(event)) |listeners| {
                for (listeners.items) |listener| {
                    listener(data);
                }
            }
        }

        pub fn listenerCount(self: *Self, event: EventType) usize {
            return if (self.listeners.get(event)) |listeners| listeners.items.len else 0;
        }
    };
}

