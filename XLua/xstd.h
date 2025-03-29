#ifndef XSTD_H
#define XSTD_H 

#include "deps/lua.h" 
#include "deps/lauxlib.h" 
#include "deps/lualib.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

typedef enum {
    T_CHARP, 
    T_INT, 
    T_VOIDP,
    T_CHAR,
    T_FLOAT,
    T_DOUBLE,
    T_INT8,
    T_INT16,
    T_INT32,
    T_INT64,
    T_UINT8,
    T_UINT16,
    T_UINT32,
    T_UINT64,
} LPTypes;

typedef struct {
    LPTypes initial_type;
    LPTypes current_type;
    void *ptr;
    size_t size;
    size_t capacity;
    int is_array;
} LPointer;

typedef struct {
    char buffer[8388608];    
    size_t size;
    size_t capacity;
    LPointer pointers[1024];
    size_t pointer_count;
} LPArena;

static LPArena global_arena = {
    .buffer = {0},
    .size = 0,
    .capacity = 8388608,
    .pointers = {0},
    .pointer_count = 0
};

size_t type_size(LPTypes type) {
    switch (type) {
        case T_CHAR:   return sizeof(char);
        case T_INT:    return sizeof(int);
        case T_FLOAT:  return sizeof(float);
        case T_DOUBLE: return sizeof(double);
        case T_CHARP:  return sizeof(char*);
        case T_VOIDP:  return sizeof(void*);
        case T_INT8:   return sizeof(int8_t);
        case T_INT16:  return sizeof(int16_t);
        case T_INT32:  return sizeof(int32_t);
        case T_INT64:  return sizeof(int64_t);
        case T_UINT8:  return sizeof(uint8_t);
        case T_UINT16: return sizeof(uint16_t);
        case T_UINT32: return sizeof(uint32_t);
        case T_UINT64: return sizeof(uint64_t);
        default:       return 0;
    }
}

const char* type_name(LPTypes type) {
    switch (type) {
        case T_CHAR:   return "char";
        case T_INT:    return "int";
        case T_FLOAT:  return "float";
        case T_DOUBLE: return "double";
        case T_CHARP:  return "char*";
        case T_VOIDP:  return "void*";
        case T_INT8:   return "int8";
        case T_INT16:  return "int16";
        case T_INT32:  return "int32";
        case T_INT64:  return "int64";
        case T_UINT8:  return "uint8";
        case T_UINT16: return "uint16";
        case T_UINT32: return "uint32";
        case T_UINT64: return "uint64";
        default:       return "unknown";
    }
}

LPTypes string_to_type(const char* type_str) {
    if (strcmp(type_str, "char") == 0)       return T_CHAR;
    if (strcmp(type_str, "int") == 0)        return T_INT;
    if (strcmp(type_str, "float") == 0)      return T_FLOAT;
    if (strcmp(type_str, "double") == 0)     return T_DOUBLE;
    if (strcmp(type_str, "char*") == 0)      return T_CHARP;
    if (strcmp(type_str, "void*") == 0)      return T_VOIDP;
    if (strcmp(type_str, "int8") == 0)       return T_INT8;
    if (strcmp(type_str, "int16") == 0)      return T_INT16;
    if (strcmp(type_str, "int32") == 0)      return T_INT32;
    if (strcmp(type_str, "int64") == 0)      return T_INT64;
    if (strcmp(type_str, "uint8") == 0)      return T_UINT8;
    if (strcmp(type_str, "uint16") == 0)     return T_UINT16;
    if (strcmp(type_str, "uint32") == 0)     return T_UINT32;
    if (strcmp(type_str, "uint64") == 0)     return T_UINT64;
    
    return T_VOIDP;
}

void xstd_push_function(lua_State *L, lua_CFunction f, const char *name) {
    lua_pushcfunction(L, f);
    lua_setfield(L, -2, name);
}

static LPointer* find_pointer(void* ptr) {
    for (size_t i = 0; i < global_arena.pointer_count; i++) {
        if (global_arena.pointers[i].ptr == ptr) {
            return &global_arena.pointers[i];
        }
    }
    return NULL;
}

static void register_pointer(LPointer pointer) {
    if (global_arena.pointer_count >= 1024) {
        return;
    }
    
    global_arena.pointers[global_arena.pointer_count++] = pointer;
}

static int lua_malloc(lua_State *L) {
    size_t size = luaL_checkinteger(L, 1);
    const char* type_str = luaL_optstring(L, 2, "void*");
    int is_array = lua_toboolean(L, 3);
    
    if (global_arena.size + size > global_arena.capacity) {
        return luaL_error(L, "arena overflow");
    }
    
    LPTypes type = string_to_type(type_str);
    size_t actual_size = is_array ? size : type_size(type);
    
    if (global_arena.size + actual_size > global_arena.capacity) {
        return luaL_error(L, "arena overflow");
    }
    
    void* ptr = &global_arena.buffer[global_arena.size];
    global_arena.size += actual_size;
    
    LPointer pointer = {
        .initial_type = type,
        .current_type = type,
        .ptr = ptr,
        .size = actual_size,
        .capacity = actual_size,
        .is_array = is_array
    };
    
    register_pointer(pointer);
    
    lua_pushlightuserdata(L, ptr);
    return 1;
}

static int lua_realloc(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    size_t new_size = luaL_checkinteger(L, 2);
    
    LPointer* pointer = find_pointer(ptr);
    if (!pointer) {
        return luaL_error(L, "pointer not found in registry");
    }
    
    if ((char*)ptr + pointer->size == &global_arena.buffer[global_arena.size]) {
        size_t size_diff = new_size - pointer->size;
        
        if (global_arena.size + size_diff > global_arena.capacity) {
            return luaL_error(L, "arena overflow during realloc");
        }
        
        global_arena.size += size_diff;
        pointer->size = new_size;
        pointer->capacity = new_size;
        
        lua_pushlightuserdata(L, ptr);
        return 1;
    }
    
    if (global_arena.size + new_size > global_arena.capacity) {
        return luaL_error(L, "arena overflow during realloc");
    }
    
    void* new_ptr = &global_arena.buffer[global_arena.size];
    memcpy(new_ptr, ptr, pointer->size < new_size ? pointer->size : new_size);
    global_arena.size += new_size;
    
    pointer->ptr = new_ptr;
    pointer->size = new_size;
    pointer->capacity = new_size;
    
    lua_pushlightuserdata(L, new_ptr);
    return 1;
}

static int lua_memcpy(lua_State *L) {
    void *dest = lua_touserdata(L, 1);
    void *src = lua_touserdata(L, 2);
    size_t n = luaL_checkinteger(L, 3);
    
    if (!dest || !src) {
        return luaL_error(L, "invalid pointers for memcpy");
    }
    
    memcpy(dest, src, n);
    return 0;
}

static int lua_memset(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    int value = luaL_checkinteger(L, 2);
    size_t n = luaL_checkinteger(L, 3);
    
    if (!ptr) {
        return luaL_error(L, "invalid pointer for memset");
    }
    
    memset(ptr, value, n);
    return 0;
}

static int lua_assign(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    int offset = luaL_optinteger(L, 2, 0);
    const char* type_str = luaL_checkstring(L, 3);
    
    if (!ptr) {
        return luaL_error(L, "invalid pointer for assignment");
    }
    
    LPTypes type = string_to_type(type_str);
    char* target = (char*)ptr + offset;
    
    switch (type) {
        case T_CHAR: {
            char value = (char)luaL_checkinteger(L, 4);
            *((char*)target) = value;
            break;
        }
        case T_INT: {
            int value = luaL_checkinteger(L, 4);
            *((int*)target) = value;
            break;
        }
        case T_FLOAT: {
            float value = (float)luaL_checknumber(L, 4);
            *((float*)target) = value;
            break;
        }
        case T_DOUBLE: {
            double value = luaL_checknumber(L, 4);
            *((double*)target) = value;
            break;
        }
        case T_CHARP:
        case T_VOIDP: {
            void* value = lua_touserdata(L, 4);
            *((void**)target) = value;
            break;
        }
        case T_INT8: {
            int8_t value = (int8_t)luaL_checkinteger(L, 4);
            *((int8_t*)target) = value;
            break;
        }
        case T_INT16: {
            int16_t value = (int16_t)luaL_checkinteger(L, 4);
            *((int16_t*)target) = value;
            break;
        }
        case T_INT32: {
            int32_t value = (int32_t)luaL_checkinteger(L, 4);
            *((int32_t*)target) = value;
            break;
        }
        case T_INT64: {
            int64_t value = (int64_t)luaL_checkinteger(L, 4);
            *((int64_t*)target) = value;
            break;
        }
        case T_UINT8: {
            uint8_t value = (uint8_t)luaL_checkinteger(L, 4);
            *((uint8_t*)target) = value;
            break;
        }
        case T_UINT16: {
            uint16_t value = (uint16_t)luaL_checkinteger(L, 4);
            *((uint16_t*)target) = value;
            break;
        }
        case T_UINT32: {
            uint32_t value = (uint32_t)luaL_checkinteger(L, 4);
            *((uint32_t*)target) = value;
            break;
        }
        case T_UINT64: {
            uint64_t value = (uint64_t)luaL_checkinteger(L, 4);
            *((uint64_t*)target) = value;
            break;
        }
        default:
            return luaL_error(L, "unsupported type for assignment");
    }
    
    return 0;
}

static int lua_deref(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    int offset = luaL_optinteger(L, 2, 0);
    const char* type_str = luaL_checkstring(L, 3);
    
    if (!ptr) {
        return luaL_error(L, "invalid pointer for dereferencing");
    }
    
    LPTypes type = string_to_type(type_str);
    char* source = (char*)ptr + offset;
    
    switch (type) {
        case T_CHAR: {
            char value = *((char*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_INT: {
            int value = *((int*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_FLOAT: {
            float value = *((float*)source);
            lua_pushnumber(L, value);
            break;
        }
        case T_DOUBLE: {
            double value = *((double*)source);
            lua_pushnumber(L, value);
            break;
        }
        case T_CHARP:
        case T_VOIDP: {
            void* value = *((void**)source);
            lua_pushlightuserdata(L, value);
            break;
        }
        case T_INT8: {
            int8_t value = *((int8_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_INT16: {
            int16_t value = *((int16_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_INT32: {
            int32_t value = *((int32_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_INT64: {
            int64_t value = *((int64_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_UINT8: {
            uint8_t value = *((uint8_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_UINT16: {
            uint16_t value = *((uint16_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_UINT32: {
            uint32_t value = *((uint32_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        case T_UINT64: {
            uint64_t value = *((uint64_t*)source);
            lua_pushinteger(L, value);
            break;
        }
        default:
            return luaL_error(L, "unsupported type for dereferencing");
    }
    
    return 1;
}

static int lua_strdup(lua_State *L) {
    const char* str = luaL_checkstring(L, 1);
    size_t len = strlen(str) + 1;
    
    if (global_arena.size + len > global_arena.capacity) {
        return luaL_error(L, "arena overflow during strdup");
    }
    
    char* new_str = (char*)&global_arena.buffer[global_arena.size];
    strcpy(new_str, str);
    global_arena.size += len;
    
    LPointer pointer = {
        .initial_type = T_CHARP,
        .current_type = T_CHARP,
        .ptr = new_str,
        .size = len,
        .capacity = len,
        .is_array = 1
    };
    
    register_pointer(pointer);
    
    lua_pushlightuserdata(L, new_str);
    return 1;
}

static int lua_sizeof(lua_State *L) {
    const char* type_str = luaL_checkstring(L, 1);
    LPTypes type = string_to_type(type_str);
    
    lua_pushinteger(L, type_size(type));
    return 1;
}

static int lua_get_pointer_info(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    
    if (!ptr) {
        return luaL_error(L, "invalid pointer");
    }
    
    LPointer* pointer = find_pointer(ptr);
    if (!pointer) {
        lua_pushnil(L);
        return 1;
    }
    
    lua_newtable(L);
    
    lua_pushstring(L, type_name(pointer->initial_type));
    lua_setfield(L, -2, "initial_type");
    
    lua_pushstring(L, type_name(pointer->current_type));
    lua_setfield(L, -2, "current_type");
    
    lua_pushinteger(L, pointer->size);
    lua_setfield(L, -2, "size");
    
    lua_pushinteger(L, pointer->capacity);
    lua_setfield(L, -2, "capacity");
    
    lua_pushboolean(L, pointer->is_array);
    lua_setfield(L, -2, "is_array");
    
    return 1;
}

static int lua_cast(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    const char* type_str = luaL_checkstring(L, 2);
    
    if (!ptr) {
        return luaL_error(L, "invalid pointer for casting");
    }
    
    LPTypes type = string_to_type(type_str);
    
    LPointer* pointer = find_pointer(ptr);
    if (pointer) {
        pointer->current_type = type;
    }
    
    lua_pushlightuserdata(L, ptr);
    return 1;
}

static int lua_arena_reset(lua_State *L) {
    global_arena.size = 0;
    global_arena.pointer_count = 0;
    
    return 0;
}

static int lua_pointer_add(lua_State *L) {
    void *ptr = lua_touserdata(L, 1);
    int offset = luaL_checkinteger(L, 2);
    
    if (!ptr) {
        return luaL_error(L, "invalid pointer for addition");
    }
    
    void* result = (char*)ptr + offset;
    lua_pushlightuserdata(L, result);
    return 1;
}

static int lua_arena_size(lua_State *L) {
    lua_pushinteger(L, global_arena.size);
    return 1;
}

static int lua_arena_capacity(lua_State *L) {
    lua_pushinteger(L, global_arena.capacity);
    return 1;
}

static int lua_arena_ptr(lua_State *L) {
    lua_pushlightuserdata(L, global_arena.buffer);
    return 1;
}

static int lua_arena_used(lua_State *L) {
    lua_pushnumber(L, (double)global_arena.size / (double)global_arena.capacity);
    return 1;
}

void xstd_init(lua_State *L) {
    lua_newtable(L);
    
    xstd_push_function(L, lua_malloc, "malloc");
    xstd_push_function(L, lua_realloc, "realloc");
    xstd_push_function(L, lua_memcpy, "memcpy");
    xstd_push_function(L, lua_memset, "memset");
    xstd_push_function(L, lua_assign, "assign");
    xstd_push_function(L, lua_deref, "deref");
    xstd_push_function(L, lua_strdup, "strdup");
    xstd_push_function(L, lua_sizeof, "sizeof");
    xstd_push_function(L, lua_get_pointer_info, "get_info");
    xstd_push_function(L, lua_cast, "cast");
    xstd_push_function(L, lua_arena_reset, "reset");
    xstd_push_function(L, lua_pointer_add, "add");
    xstd_push_function(L, lua_arena_size, "arena_size");
    xstd_push_function(L, lua_arena_capacity, "arena_capacity");
    xstd_push_function(L, lua_arena_ptr, "arena_ptr");
    xstd_push_function(L, lua_arena_used, "arena_used");
    
    lua_setglobal(L, "std");
    
    global_arena.size = 0;
    global_arena.capacity = sizeof(global_arena.buffer);
    global_arena.pointer_count = 0;
}

#endif // XSTD_H
