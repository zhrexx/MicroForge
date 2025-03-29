local luaptr = {}

local memory_blocks = {}
local next_memory_id = 1
local allocated_pointers = {}
local base_address = 0x1000  

local type_sizes = {
  int = 4,
  float = 4,
  double = 8,
  char = 1,
  boolean = 1,
  string = 8,
  table = 8,
  any = 8
}

local function generate_address(id, offset)
  local block_address = base_address + (id * 0x100000)
  return block_address + offset
end

local function check_ptr(ptr, operation)
  if not ptr or type(ptr) ~= "table" or not ptr._is_ptr then
    error(string.format("Invalid pointer for %s operation", operation or "pointer"))
  end
end

local function check_memory_block(ptr, operation)
  check_ptr(ptr, operation)
  if not memory_blocks[ptr._id] then
    error(string.format("Null pointer exception: %s", operation or "access"))
  end
end

local function get_index(ptr)
  return math.floor(ptr._offset / luaptr.sizeof(ptr._type)) + 1
end

local function get_physical_address(ptr)
  return generate_address(ptr._id, ptr._offset)
end

local pointer_mt = {
  __add = function(ptr, offset)
    check_ptr(ptr, "addition")
    if type(offset) ~= "number" then
      error("Pointer arithmetic requires a number offset")
    end
    
    local new_ptr = luaptr.copy(ptr)
    new_ptr._offset = ptr._offset + offset * luaptr.sizeof(ptr._type)
    return new_ptr
  end,
  
  __sub = function(ptr, offset)
    check_ptr(ptr, "subtraction")
    if type(offset) == "number" then
      local new_ptr = luaptr.copy(ptr)
      new_ptr._offset = ptr._offset - offset * luaptr.sizeof(ptr._type)
      return new_ptr
    elseif type(offset) == "table" and offset._is_ptr then
      if ptr._id ~= offset._id then
        error("Cannot perform arithmetic on pointers to different memory blocks")
      end
      return (ptr._offset - offset._offset) / luaptr.sizeof(ptr._type)
    else
      error("Invalid pointer subtraction")
    end
  end,
  
  __eq = function(ptr1, ptr2)
    check_ptr(ptr1, "comparison")
    check_ptr(ptr2, "comparison")
    return ptr1._id == ptr2._id and ptr1._offset == ptr2._offset
  end,
  
  __lt = function(ptr1, ptr2)
    check_ptr(ptr1, "comparison")
    check_ptr(ptr2, "comparison")
    if ptr1._id ~= ptr2._id then
      error("Cannot compare pointers to different memory blocks")
    end
    return ptr1._offset < ptr2._offset
  end,
  
  __le = function(ptr1, ptr2)
    check_ptr(ptr1, "comparison")
    check_ptr(ptr2, "comparison")
    if ptr1._id ~= ptr2._id then
      error("Cannot compare pointers to different memory blocks")
    end
    return ptr1._offset <= ptr2._offset
  end,
  
  __tostring = function(ptr)
    check_ptr(ptr, "tostring")
    if ptr._id == 0 then
      return string.format("0x0 (NULL<%s>)", ptr._type)
    end
    return string.format("0x%08X (%s*)", get_physical_address(ptr), ptr._type)
  end,
  
  __index = function(ptr, key)
    check_ptr(ptr, "indexing")
    if key == "val" or key == "value" then
      return luaptr.deref(ptr)
    elseif key == "ptr" or key == "address" then
      return ptr
    elseif key == "addr" or key == "hex" then
      return string.format("0x%08X", get_physical_address(ptr))
    elseif type(key) == "number" then
      return luaptr.deref(ptr + key)
    end
  end,
  
  __newindex = function(ptr, key, value)
    check_ptr(ptr, "assignment")
    if key == "val" or key == "value" then
      luaptr.set(ptr, value)
    elseif type(key) == "number" then
      luaptr.set(ptr + key, value)
    end
  end
}

function luaptr.new(type_name)
  type_name = type_name or "any"
  
  local ptr = {
    _is_ptr = true,
    _type = type_name,
    _id = 0,
    _offset = 0
  }
  
  return setmetatable(ptr, pointer_mt)
end

function luaptr.ptr(value, type_name)
  type_name = type_name or type(value)
  
  local ptr = luaptr.new(type_name)
  local mem_id = next_memory_id
  next_memory_id = next_memory_id + 1
  
  memory_blocks[mem_id] = {value}
  ptr._id = mem_id
  
  allocated_pointers[ptr] = true
  
  return ptr
end

function luaptr.deref(ptr)
  check_memory_block(ptr, "dereference")
  
  local mem_block = memory_blocks[ptr._id]
  local index = get_index(ptr)
  
  if index < 1 or index > #mem_block then
    error(string.format("Segmentation fault: invalid memory access at address 0x%08X", get_physical_address(ptr)))
  end
  
  return mem_block[index]
end

function luaptr.set(ptr, value)
  check_memory_block(ptr, "set")
  
  local mem_block = memory_blocks[ptr._id]
  local index = get_index(ptr)
  
  if index < 1 or index > #mem_block then
    error(string.format("Segmentation fault: invalid memory access at address 0x%08X", get_physical_address(ptr)))
  end
  
  mem_block[index] = value
  return value
end

function luaptr.sizeof(type_name)
  return type_sizes[type_name] or type_sizes.any
end

function luaptr.set_type_size(type_name, size)
  if type(size) ~= "number" or size <= 0 then
    error("Type size must be a positive number")
  end
  type_sizes[type_name] = size
end

function luaptr.malloc(size_bytes, type_name)
  type_name = type_name or "char"
  
  if type(size_bytes) ~= "number" or size_bytes <= 0 then
    error("Size must be a positive number")
  end
  
  local mem_id = next_memory_id
  next_memory_id = next_memory_id + 1
  
  local element_size = luaptr.sizeof(type_name)
  local num_elements = math.ceil(size_bytes / element_size)
  
  local mem_block = {}
  for i = 1, num_elements do
    mem_block[i] = nil
  end
  memory_blocks[mem_id] = mem_block
  
  local ptr = luaptr.new(type_name)
  ptr._id = mem_id
  
  allocated_pointers[ptr] = true
  
  return ptr
end

function luaptr.free(ptr)
  check_ptr(ptr, "free")
  
  if ptr._id == 0 then
    return false
  end
  
  memory_blocks[ptr._id] = nil
  allocated_pointers[ptr] = nil
  ptr._id = 0
  
  return true
end

function luaptr.array(size, type_name, init_value)
  type_name = type_name or "any"
  
  if type(size) ~= "number" or size <= 0 then
    error("Array size must be a positive number")
  end
  
  local mem_id = next_memory_id
  next_memory_id = next_memory_id + 1
  
  local mem_block = {}
  for i = 1, size do
    mem_block[i] = init_value
  end
  memory_blocks[mem_id] = mem_block
  
  local ptr = luaptr.new(type_name)
  ptr._id = mem_id
  
  allocated_pointers[ptr] = true
  
  return ptr
end

function luaptr.cast(ptr, new_type)
  check_ptr(ptr, "cast")
  
  if not new_type or type(new_type) ~= "string" then
    error("Cast requires a valid type name")
  end
  
  local new_ptr = luaptr.new(new_type)
  new_ptr._id = ptr._id
  new_ptr._offset = ptr._offset
  
  return new_ptr
end

function luaptr.null(type_name)
  return luaptr.new(type_name)
end

function luaptr.is_null(ptr)
  check_ptr(ptr, "is_null check")
  return ptr._id == 0
end

function luaptr.copy(ptr)
  check_ptr(ptr, "copy")
  
  local new_ptr = luaptr.new(ptr._type)
  new_ptr._id = ptr._id
  new_ptr._offset = ptr._offset
  
  return new_ptr
end

function luaptr.address(ptr)
  check_ptr(ptr, "address")
  return get_physical_address(ptr)
end

function luaptr.memory_stats()
  local total_blocks = 0
  local total_elements = 0
  local total_pointers = 0
  local memory_usage = 0
  
  for _ in pairs(memory_blocks) do
    total_blocks = total_blocks + 1
  end
  
  for _, block in pairs(memory_blocks) do
    local elements = 0
    for _ in pairs(block) do
      elements = elements + 1
    end
    total_elements = total_elements + elements
    memory_usage = memory_usage + (elements * 8)
  end
  
  for _ in pairs(allocated_pointers) do
    total_pointers = total_pointers + 1
  end
  
  return {
    blocks = total_blocks,
    elements = total_elements,
    pointers = total_pointers,
    memory_usage = memory_usage,
    heap_start = string.format("0x%08X", base_address)
  }
end

function luaptr.memcpy(dest_ptr, src_ptr, num_bytes)
  check_memory_block(dest_ptr, "memcpy destination")
  check_memory_block(src_ptr, "memcpy source")
  
  if type(num_bytes) ~= "number" or num_bytes <= 0 then
    error("Bytes to copy must be a positive number")
  end
  
  local src_block = memory_blocks[src_ptr._id]
  local dest_block = memory_blocks[dest_ptr._id]
  
  local src_type_size = luaptr.sizeof(src_ptr._type)
  local dest_type_size = luaptr.sizeof(dest_ptr._type)
  
  local src_start = get_index(src_ptr)
  local dest_start = get_index(dest_ptr)
  
  local elements = math.floor(num_bytes / src_type_size)
  
  for i = 0, elements - 1 do
    local src_index = src_start + i
    local dest_index = dest_start + i
    
    if src_index <= #src_block and dest_index <= #dest_block then
      dest_block[dest_index] = src_block[src_index]
    else
      error(string.format("Memory copy out of bounds: 0x%08X -> 0x%08X", 
        get_physical_address(src_ptr) + (i * src_type_size),
        get_physical_address(dest_ptr) + (i * dest_type_size)))
    end
  end
  
  return dest_ptr
end

function luaptr.memset(ptr, value, num_bytes)
  check_memory_block(ptr, "memset")
  
  if type(num_bytes) ~= "number" or num_bytes <= 0 then
    error("Bytes to set must be a positive number")
  end
  
  local block = memory_blocks[ptr._id]
  local type_size = luaptr.sizeof(ptr._type)
  local elements = math.floor(num_bytes / type_size)
  local start_index = get_index(ptr)
  
  for i = 0, elements - 1 do
    local index = start_index + i
    if index <= #block then
      block[index] = value
    else
      error(string.format("Memory set out of bounds at address 0x%08X", 
        get_physical_address(ptr) + (i * type_size)))
    end
  end
  
  return ptr
end

function luaptr.length(ptr)
  check_memory_block(ptr, "length")
  
  return #memory_blocks[ptr._id]
end

function luaptr.valid(ptr)
  if not ptr or type(ptr) ~= "table" or not ptr._is_ptr then
    return false
  end
  
  return memory_blocks[ptr._id] ~= nil
end

function luaptr.realloc(ptr, new_size)
  check_memory_block(ptr, "realloc")
  
  if type(new_size) ~= "number" or new_size < 0 then
    error("New size must be a non-negative number")
  end
  
  local old_block = memory_blocks[ptr._id]
  local type_size = luaptr.sizeof(ptr._type)
  local new_elements = math.ceil(new_size / type_size)
  
  if new_elements == #old_block then
    return ptr
  end
  
  local new_ptr = luaptr.malloc(new_size, ptr._type)
  local new_block = memory_blocks[new_ptr._id]
  
  for i = 1, math.min(#old_block, new_elements) do
    new_block[i] = old_block[i]
  end
  
  luaptr.free(ptr)
  return new_ptr
end

function luaptr.set_base_address(addr)
  if type(addr) == "number" and addr >= 0 then
    base_address = addr
    return true
  end
  return false
end

function luaptr.get_address(ptr)
  check_ptr(ptr, "get_address")
  return get_physical_address(ptr)
end

function luaptr.dump_memory(ptr, size)
  check_memory_block(ptr, "memory dump")
  
  size = size or 16
  if type(size) ~= "number" or size <= 0 then
    error("Size must be a positive number")
  end
  
  local mem_block = memory_blocks[ptr._id]
  local start_index = get_index(ptr)
  local base_addr = get_physical_address(ptr)
  local output = {}
  
  for i = 0, size - 1 do
    local index = start_index + i
    local value = index <= #mem_block and mem_block[index] or nil
    local val_str = "??"
    
    if value ~= nil then
      if type(value) == "number" then
        val_str = string.format("%02X", value % 256)
      elseif type(value) == "string" and #value == 1 then
        val_str = string.format("%02X", string.byte(value))
      elseif type(value) == "boolean" then
        val_str = value and "01" or "00"
      else
        val_str = "??"
      end
    endWrite a Compiler that compiles lua to xbin bytecode and make a js library to load the bytecode use the liblua and write it in c(compiler) full Code no comments in code
The js library should work in Browser no dependecies in js
Make a completely unique bytecode and dont use wasm
    
    if i % 16 == 0 then
      table.insert(output, string.format("\n0x%08X: ", base_addr + i))
    end
    
    table.insert(output, val_str .. " ")
  end
  
  return table.concat(output)
end

function luaptr.cleanup()
  memory_blocks = {}
  allocated_pointers = {}
  next_memory_id = 1
  base_address = 0x1000
end

luaptr.dereference_mt = {
  __call = function(_, ptr)
    return luaptr.deref(ptr)
  end
}

luaptr.val = setmetatable({}, luaptr.dereference_mt)

return luaptr
