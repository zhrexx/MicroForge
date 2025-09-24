# MicroForge Libraries

This directory contains useful C libraries for the MicroForge project. All libraries are header-only for easy integration.

## Available Libraries

### 1. StringBuilder (`sb.h`)
A dynamic string building library that allows efficient string concatenation and manipulation.

**Key Features:**
- Dynamic resizing
- Efficient append operations
- Slice operations
- Convert to C string

### 2. StringArray (`sarr.h`) 
A dynamic array of strings with automatic memory management.

**Key Features:**
- Dynamic string array
- Automatic resizing
- String combination
- Token splitting from delimited strings

### 3. Smart Pointer (`spointer.h`)
A reference-counted smart pointer implementation for C.

**Key Features:**
- Reference counting
- Automatic memory management
- Copy/move semantics
- Aligned allocation support

### 4. HashMap (`hmap.h`) ⭐ **NEW**
A generic hash table implementation with string keys and typed values.

**Key Features:**
- Generic key-value storage
- Automatic resizing
- Collision resolution via chaining
- Type-safe convenience functions for common types
- Key enumeration
- O(1) average-case operations

## Hash Map Library (`hmap.h`)

### Quick Start

```c
#include "hmap.h"

int main() {
    // Create a hash map
    HashMap map = hmap_create(0);  // 0 = default capacity
    
    // Store different types of data
    hmap_put_string(&map, "name", "John Doe");
    hmap_put_int(&map, "age", 30);
    hmap_put_double(&map, "salary", 75000.50);
    
    // Retrieve data
    const char* name = hmap_get_string(&map, "name");
    int* age = hmap_get_int(&map, "age");
    double* salary = hmap_get_double(&map, "salary");
    
    printf("Name: %s, Age: %d, Salary: $%.2f\n", 
           name, *age, *salary);
    
    // Clean up
    hmap_destroy(&map);
    return 0;
}
```

### Core Functions

#### Map Management
- `HashMap hmap_create(size_t initial_capacity)` - Create new hash map
- `void hmap_destroy(HashMap *map)` - Free all memory
- `void hmap_clear(HashMap *map)` - Remove all entries
- `size_t hmap_size(HashMap *map)` - Get number of entries
- `bool hmap_is_empty(HashMap *map)` - Check if empty

#### Data Operations
- `bool hmap_put(HashMap *map, const char *key, const void *value, size_t value_size)` - Store generic data
- `void* hmap_get(HashMap *map, const char *key, size_t *value_size)` - Retrieve generic data
- `bool hmap_contains(HashMap *map, const char *key)` - Check if key exists
- `bool hmap_remove(HashMap *map, const char *key)` - Remove entry

#### Key Management
- `char** hmap_keys(HashMap *map, size_t *count)` - Get all keys
- `void hmap_free_keys(char **keys, size_t count)` - Free keys array

#### Type-Safe Convenience Functions
- `hmap_put_string()` / `hmap_get_string()` - String values
- `hmap_put_int()` / `hmap_get_int()` - Integer values  
- `hmap_put_double()` / `hmap_get_double()` - Double values

### Use Cases

1. **Configuration Management** - Store application settings
2. **Caching** - Cache computed results for performance
3. **User Data** - Store user profiles and preferences
4. **Lookup Tables** - Fast key-based data retrieval
5. **Word Counting** - Text analysis and statistics
6. **Database Records** - In-memory record storage

### Performance Characteristics

- **Time Complexity**: O(1) average case for all operations
- **Space Complexity**: O(n) where n is number of entries
- **Load Factor**: Automatically resizes at 75% capacity
- **Collision Resolution**: Separate chaining with linked lists
- **Hash Function**: djb2 algorithm for good distribution

### Memory Management

The hash map handles all memory allocation and deallocation automatically:
- Keys are duplicated and stored internally
- Values are copied to internal storage
- All memory is freed when `hmap_destroy()` is called
- No memory leaks when used correctly

### Thread Safety

⚠️ **Not thread-safe** - External synchronization required for concurrent access.

## Testing

Each library includes comprehensive tests:

```bash
# Test the hash map library
cd libraries
gcc -o hmap_test hmap_test.c && ./hmap_test

# Run practical examples
gcc -o hmap_example hmap_example.c && ./hmap_example
```

## Integration

All libraries are header-only. Simply include the header file in your project:

```c
#include "libraries/hmap.h"
#include "libraries/sb.h"
#include "libraries/sarr.h"
#include "libraries/spointer.h"
```

## License

These libraries are part of the MicroForge project. See the main project LICENSE file for details.