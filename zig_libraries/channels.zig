const std = @import("std");
const Allocator = std.mem.Allocator;
const Mutex = std.Thread.Mutex;
const Condition = std.Thread.Condition;

pub fn Channel(comptime T: type) type {
    return struct {
        const Self = @This();

        /// Errors that can occur when using the channel.
        pub const Error = error{
        /// The channel is closed.
            Closed,
        /// The channel is empty (for tryReceive).
            Empty,
        /// The channel is full (for trySend with bounded capacity).
            Full,
        /// Memory allocation failed.
            OutOfMemory,
        };

        queue: std.DoublyLinkedList(T),
        mutex: Mutex,
        not_empty: Condition,
        not_full: Condition,
        allocator: Allocator,
        capacity: ?usize,
        closed: bool,
        count: usize,

        /// Initializes a new channel with the given allocator and capacity.
        /// If capacity is null, the channel is unbounded.
        /// If capacity is 0, the channel is synchronous (send blocks until receive).
        /// If capacity > 0, the channel is bounded to that size.
        /// Note: For capacity == 0, the implementation may not fully support synchronous behavior.
        pub fn init(allocator: Allocator, capacity: ?usize) Self {
            return Self{
                .queue = std.DoublyLinkedList(T){},
                .mutex = Mutex{},
                .not_empty = Condition{},
                .not_full = Condition{},
                .allocator = allocator,
                .capacity = capacity,
                .closed = false,
                .count = 0,
            };
        }

        /// Deinitializes the channel, freeing all allocated memory.
        pub fn deinit(self: *Self) void {
            self.mutex.lock();
            defer self.mutex.unlock();

            while (self.queue.popFirst()) |node| {
                self.allocator.destroy(node);
            }
        }

        /// Sends a value to the channel.
        /// Blocks if the channel is bounded and full.
        /// Returns an error if the channel is closed.
        pub fn send(self: *Self, value: T) Error!void {
            self.mutex.lock();
            defer self.mutex.unlock();

            if (self.closed) {
                return Error.Closed;
            }

            if (self.capacity) |capacity| {
                while (self.count >= capacity and !self.closed) {
                    self.not_full.wait(&self.mutex);
                }
                if (self.closed) {
                    return Error.Closed;
                }
            }

            var node = self.allocator.create(std.DoublyLinkedList(T).Node) catch {
                return Error.OutOfMemory;
            };
            node.data = value;
            self.queue.append(node);
            self.count += 1;

            self.not_empty.signal();

            return;
        }

        /// Tries to send a value without blocking.
        /// Returns an error if the channel is closed or full.
        pub fn trySend(self: *Self, value: T) Error!void {
            self.mutex.lock();
            defer self.mutex.unlock();

            if (self.closed) {
                return Error.Closed;
            }

            if (self.capacity) |capacity| {
                if (self.count >= capacity) {
                    return Error.Full;
                }
            }

            var node = self.allocator.create(std.DoublyLinkedList(T).Node) catch {
                return Error.OutOfMemory;
            };
            node.data = value;
            self.queue.append(node);
            self.count += 1;

            self.not_empty.signal();

            return;
        }

        /// Receives a value from the channel.
        /// Blocks if the channel is empty.
        /// Returns an error if the channel is closed and empty.
        pub fn receive(self: *Self) Error!T {
            self.mutex.lock();
            defer self.mutex.unlock();

            while (self.count == 0 and !self.closed) {
                self.not_empty.wait(&self.mutex);
            }

            if (self.count == 0 and self.closed) {
                return Error.Closed;
            }

            const node = self.queue.popFirst().?;
            const value = node.data;
            self.allocator.destroy(node);
            self.count -= 1;

            self.not_full.signal();

            return value;
        }

        /// Tries to receive a value without blocking.
        /// Returns an error if the channel is empty or closed.
        pub fn tryReceive(self: *Self) Error!T {
            self.mutex.lock();
            defer self.mutex.unlock();

            if (self.count > 0) {
                const node = self.queue.popFirst().?;
                const value = node.data;
                self.allocator.destroy(node);
                self.count -= 1;
                self.not_full.signal();
                return value;
            } else if (self.closed) {
                return Error.Closed;
            } else {
                return Error.Empty;
            }
        }

        /// Closes the channel.
        /// After closing, send operations will fail, but receive operations
        /// will continue to work until the channel is empty.
        pub fn close(self: *Self) void {
            self.mutex.lock();
            defer self.mutex.unlock();

            self.closed = true;

            self.not_empty.broadcast();
            self.not_full.broadcast();
        }

        /// Returns true if the channel is closed.
        pub fn isClosed(self: *Self) bool {
            self.mutex.lock();
            defer self.mutex.unlock();

            return self.closed;
        }

        /// Returns the number of items in the channel.
        pub fn len(self: *Self) usize {
            self.mutex.lock();
            defer self.mutex.unlock();

            return self.count;
        }

        /// Returns true if the channel is empty.
        pub fn isEmpty(self: *Self) bool {
            self.mutex.lock();
            defer self.mutex.unlock();

            return self.count == 0;
        }
    };
}

pub fn createChannel(comptime T: type, allocator: Allocator) Channel(T) {
    return Channel(T).init(allocator, null);
}

pub fn createBoundedChannel(comptime T: type, allocator: Allocator, capacity: usize) Channel(T) {
    return Channel(T).init(allocator, capacity);
}

pub fn createSynchronousChannel(comptime T: type, allocator: Allocator) Channel(T) {
    return Channel(T).init(allocator, 0);
}