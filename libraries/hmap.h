#ifndef HMAP_H
#define HMAP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// Hash Map - A generic hash table implementation
// Supports string keys with void* values for maximum flexibility

#define HMAP_INITIAL_CAPACITY 16
#define HMAP_LOAD_FACTOR 0.75
#define HMAP_RESIZE_FACTOR 2

typedef struct hmap_entry {
    char *key;
    void *value;
    size_t value_size;
    struct hmap_entry *next;  // For chaining collision resolution
} HMapEntry;

typedef struct {
    HMapEntry **buckets;
    size_t capacity;
    size_t size;
    size_t threshold;  // Resize when size reaches this threshold
} HashMap;

// Hash function - simple djb2 algorithm
static inline unsigned long hmap_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Initialize a new hash map
HashMap hmap_create(size_t initial_capacity) {
    if (initial_capacity < 1) initial_capacity = HMAP_INITIAL_CAPACITY;
    
    HashMap map = {0};
    map.capacity = initial_capacity;
    map.size = 0;
    map.threshold = (size_t)(initial_capacity * HMAP_LOAD_FACTOR);
    map.buckets = (HMapEntry**)calloc(map.capacity, sizeof(HMapEntry*));
    assert(map.buckets != NULL);
    
    return map;
}

// Free all memory used by the hash map
void hmap_destroy(HashMap *map) {
    if (!map || !map->buckets) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        HMapEntry *entry = map->buckets[i];
        while (entry) {
            HMapEntry *next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    map->buckets = NULL;
    map->capacity = 0;
    map->size = 0;
    map->threshold = 0;
}

// Create a new entry
static inline HMapEntry* hmap_create_entry(const char *key, const void *value, size_t value_size) {
    HMapEntry *entry = (HMapEntry*)malloc(sizeof(HMapEntry));
    assert(entry != NULL);
    
    entry->key = strdup(key);
    assert(entry->key != NULL);
    
    entry->value = malloc(value_size);
    assert(entry->value != NULL);
    memcpy(entry->value, value, value_size);
    
    entry->value_size = value_size;
    entry->next = NULL;
    
    return entry;
}

// Resize the hash map
static void hmap_resize(HashMap *map) {
    size_t old_capacity = map->capacity;
    HMapEntry **old_buckets = map->buckets;
    
    map->capacity *= HMAP_RESIZE_FACTOR;
    map->threshold = (size_t)(map->capacity * HMAP_LOAD_FACTOR);
    map->buckets = (HMapEntry**)calloc(map->capacity, sizeof(HMapEntry*));
    assert(map->buckets != NULL);
    
    size_t old_size = map->size;
    map->size = 0;  // Will be incremented during rehashing
    
    // Rehash all existing entries
    for (size_t i = 0; i < old_capacity; i++) {
        HMapEntry *entry = old_buckets[i];
        while (entry) {
            HMapEntry *next = entry->next;
            
            // Rehash this entry
            unsigned long hash = hmap_hash(entry->key);
            size_t index = hash % map->capacity;
            
            entry->next = map->buckets[index];
            map->buckets[index] = entry;
            map->size++;
            
            entry = next;
        }
    }
    
    free(old_buckets);
    assert(map->size == old_size);
}

// Put a key-value pair into the hash map
bool hmap_put(HashMap *map, const char *key, const void *value, size_t value_size) {
    if (!map || !key || !value) return false;
    
    unsigned long hash = hmap_hash(key);
    size_t index = hash % map->capacity;
    
    HMapEntry *entry = map->buckets[index];
    
    // Check if key already exists - update if found
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            // Key exists, update value
            free(entry->value);
            entry->value = malloc(value_size);
            assert(entry->value != NULL);
            memcpy(entry->value, value, value_size);
            entry->value_size = value_size;
            return true;
        }
        entry = entry->next;
    }
    
    // Key doesn't exist, create new entry
    HMapEntry *new_entry = hmap_create_entry(key, value, value_size);
    new_entry->next = map->buckets[index];
    map->buckets[index] = new_entry;
    map->size++;
    
    // Resize if load factor exceeded
    if (map->size >= map->threshold) {
        hmap_resize(map);
    }
    
    return true;
}

// Get a value by key from the hash map
void* hmap_get(HashMap *map, const char *key, size_t *value_size) {
    if (!map || !key) return NULL;
    
    unsigned long hash = hmap_hash(key);
    size_t index = hash % map->capacity;
    
    HMapEntry *entry = map->buckets[index];
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            if (value_size) *value_size = entry->value_size;
            return entry->value;
        }
        entry = entry->next;
    }
    
    return NULL;  // Key not found
}

// Check if a key exists in the hash map
bool hmap_contains(HashMap *map, const char *key) {
    return hmap_get(map, key, NULL) != NULL;
}

// Remove a key-value pair from the hash map
bool hmap_remove(HashMap *map, const char *key) {
    if (!map || !key) return false;
    
    unsigned long hash = hmap_hash(key);
    size_t index = hash % map->capacity;
    
    HMapEntry *entry = map->buckets[index];
    HMapEntry *prev = NULL;
    
    while (entry) {
        if (strcmp(entry->key, key) == 0) {
            // Found the entry to remove
            if (prev) {
                prev->next = entry->next;
            } else {
                map->buckets[index] = entry->next;
            }
            
            free(entry->key);
            free(entry->value);
            free(entry);
            map->size--;
            return true;
        }
        prev = entry;
        entry = entry->next;
    }
    
    return false;  // Key not found
}

// Get all keys in the hash map
char** hmap_keys(HashMap *map, size_t *count) {
    if (!map || !count) return NULL;
    
    *count = map->size;
    if (map->size == 0) return NULL;
    
    char **keys = (char**)malloc(map->size * sizeof(char*));
    assert(keys != NULL);
    
    size_t key_index = 0;
    for (size_t i = 0; i < map->capacity; i++) {
        HMapEntry *entry = map->buckets[i];
        while (entry) {
            keys[key_index++] = strdup(entry->key);
            entry = entry->next;
        }
    }
    
    return keys;
}

// Free array of keys returned by hmap_keys
void hmap_free_keys(char **keys, size_t count) {
    if (!keys) return;
    for (size_t i = 0; i < count; i++) {
        free(keys[i]);
    }
    free(keys);
}

// Clear all entries from the hash map (but keep the structure)
void hmap_clear(HashMap *map) {
    if (!map || !map->buckets) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        HMapEntry *entry = map->buckets[i];
        while (entry) {
            HMapEntry *next = entry->next;
            free(entry->key);
            free(entry->value);
            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    
    map->size = 0;
}

// Get the current size of the hash map
size_t hmap_size(HashMap *map) {
    return map ? map->size : 0;
}

// Check if the hash map is empty
bool hmap_is_empty(HashMap *map) {
    return hmap_size(map) == 0;
}

// Convenience functions for common data types

// Put a string value
static inline bool hmap_put_string(HashMap *map, const char *key, const char *value) {
    return hmap_put(map, key, value, strlen(value) + 1);
}

// Get a string value
static inline const char* hmap_get_string(HashMap *map, const char *key) {
    return (const char*)hmap_get(map, key, NULL);
}

// Put an integer value
static inline bool hmap_put_int(HashMap *map, const char *key, int value) {
    return hmap_put(map, key, &value, sizeof(int));
}

// Get an integer value (returns pointer to int, or NULL if not found)
static inline int* hmap_get_int(HashMap *map, const char *key) {
    return (int*)hmap_get(map, key, NULL);
}

// Put a double value
static inline bool hmap_put_double(HashMap *map, const char *key, double value) {
    return hmap_put(map, key, &value, sizeof(double));
}

// Get a double value (returns pointer to double, or NULL if not found)
static inline double* hmap_get_double(HashMap *map, const char *key) {
    return (double*)hmap_get(map, key, NULL);
}

#endif // HMAP_H