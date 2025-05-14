const std = @import("std");
const Allocator = std.mem.Allocator;
const Mutex = std.Thread.Mutex;
const Condition = std.Thread.Condition;

pub fn Channel(comptime T: type) type {
    return struct {
        const Self = @This();
        
        pub const Error = error{
            Closed,
            Full,
            OutOfMemory,
        };
        
        queue: std.TailQueue(T),
        mutex: Mutex,
        not_empty: Condition,
        not_full: Condition,
        allocator: Allocator,
        capacity: ?usize,
        closed: bool,
        
        /// If capacity is null, the channel is unbounded
        /// If capacity is 0, the channel is synchronous (send blocks until receive)
        /// If capacity > 0, the channel is bounded to that size
        pub fn init(allocator: Allocator, capacity: ?usize) Self {
            return Self{
                .queue = std.TailQueue(T){},
                .mutex = Mutex{},
                .not_empty = Condition{},
                .not_full = Condition{},
                .allocator = allocator,
                .capacity = capacity,
                .closed = false,
            };
        }
        
        pub fn deinit(self: *Self) void {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            while (self.queue.popFirst()) |node| {
                self.allocator.destroy(node);
            }
        }
        
        /// Sends a value to the channel
        /// Blocks if the channel is bounded and full
        /// Returns an error if the channel is closed
        pub fn send(self: *Self, value: T) Error!void {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            if (self.closed) {
                return Error.Closed;
            }
            
            if (self.capacity) |capacity| {
                var count: usize = 0;
                var it = self.queue.first;
                while (it != null) : (it = it.?.next) {
                    count += 1;
                }
                
                while (count >= capacity and !self.closed) {
                    self.not_full.wait(&self.mutex);
                    
                    if (self.closed) {
                        return Error.Closed;
                    }
                    
                    count = 0;
                    it = self.queue.first;
                    while (it != null) : (it = it.?.next) {
                        count += 1;
                    }
                }
                
                if (self.closed) {
                    return Error.Closed;
                }
            }
            
            var node = self.allocator.create(std.TailQueue(T).Node) catch {
                return Error.OutOfMemory;
            };
            node.data = value;
            self.queue.append(node);
            
            self.not_empty.signal();
            
            return;
        }
        
        /// Tries to send a value without blocking
        /// Returns an error if the channel is closed or full
        pub fn trySend(self: *Self, value: T) Error!void {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            if (self.closed) {
                return Error.Closed;
            }
            
            if (self.capacity) |capacity| {
                var count: usize = 0;
                var it = self.queue.first;
                while (it != null) : (it = it.?.next) {
                    count += 1;
                }
                
                if (count >= capacity) {
                    return Error.Full;
                }
            }
            
            var node = self.allocator.create(std.TailQueue(T).Node) catch {
                return Error.OutOfMemory;
            };
            node.data = value;
            self.queue.append(node);
            
            self.not_empty.signal();
            
            return;
        }
        
        /// Receives a value from the channel
        /// Blocks if the channel is empty
        /// Returns an error if the channel is closed and empty
        pub fn receive(self: *Self) Error!T {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            while (self.queue.first == null and !self.closed) {
                self.not_empty.wait(&self.mutex);
            }
            
            if (self.queue.first == null and self.closed) {
                return Error.Closed;
            }

            const node = self.queue.popFirst().?;
            const value = node.data;
            self.allocator.destroy(node);
            
            self.not_full.signal();
            
            return value;
        }
        
        /// Tries to receive a value without blocking
        /// Returns an error if the channel is empty
        pub fn tryReceive(self: *Self) Error!T {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            if (self.queue.first == null) {
                if (self.closed) {
                    return Error.Closed;
                } else {
                    return Error.Closed;
                }
            }
            
            const node = self.queue.popFirst().?;
            const value = node.data;
            self.allocator.destroy(node);
            
            self.not_full.signal();
            
            return value;
        }
        
        /// Closes the channel
        /// After closing, send operations will fail, but receive operations
        /// will continue to work until the channel is empty
        pub fn close(self: *Self) void {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            self.closed = true;
            
            self.not_empty.broadcast();
            self.not_full.broadcast();
        }
        
        pub fn isClosed(self: *Self) bool {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            return self.closed;
        }
        
        /// Returns the number of items in the channel
        pub fn len(self: *Self) usize {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            var count: usize = 0;
            var it = self.queue.first;
            while (it != null) : (it = it.?.next) {
                count += 1;
            }
            
            return count;
        }
        
        /// Returns true if the channel is empty
        pub fn isEmpty(self: *Self) bool {
            self.mutex.lock();
            defer self.mutex.unlock();
            
            return self.queue.first == null;
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

