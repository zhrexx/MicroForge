#ifndef XMAP_H
#define XMAP_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "word.h"

typedef struct {
    const char* key;
    Word value;
    uint32_t hash;
    uint8_t occupied;
} XEntry;

typedef struct {
    XEntry *entries;
    size_t size;
    size_t capacity;
    size_t mask;
} XMap;

static inline uint32_t xmap_hash(const char *key) {
    uint32_t hash = 2166136261u;
    while (*key) {
        hash ^= (uint8_t)(*key++);
        hash *= 16777619u;
    }
    return hash;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void xmap_init(XMap *m, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 8;
    size_t cap = 8;
    while (cap < initial_capacity) cap <<= 1;

    m->entries = calloc(cap, sizeof(XEntry));
    if (!m->entries) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    m->size = 0;
    m->capacity = cap;
    m->mask = cap - 1;
}

void xmap_free(XMap *m) {
    for (size_t i = 0; i < m->capacity; i++) {
        if (m->entries[i].occupied) {
            if (m->entries[i].key) free((void*)m->entries[i].key);
            if (m->entries[i].value.type == WPOINTER &&
                word_is_string(&m->entries[i].value) &&
                m->entries[i].value.as_pointer != NULL) {
                free(m->entries[i].value.as_pointer);
            }
        }
    }
    free(m->entries);
    m->entries = NULL;
    m->size = 0;
    m->capacity = 0;
}

///////////////////////////////////////
/// User API
///////////////////////////////////////

XMap xmap_create(size_t initial_capacity) {
    XMap result = {0};
    xmap_init(&result, initial_capacity);
    return result;
}

void xmap_resize(XMap *m, size_t new_capacity) {
    size_t cap = 8;
    while (cap < new_capacity) cap <<= 1;

    XEntry *old_entries = m->entries;
    size_t old_capacity = m->capacity;

    m->entries = calloc(cap, sizeof(XEntry));
    if (!m->entries) {
        fprintf(stderr, "Failed to reallocate memory\n");
        exit(EXIT_FAILURE);
    }
    m->capacity = cap;
    m->mask = cap - 1;
    m->size = 0;

    for (size_t i = 0; i < old_capacity; i++) {
        if (old_entries[i].occupied) {
            uint32_t idx = old_entries[i].hash & m->mask;
            while (m->entries[idx].occupied) {
                idx = (idx + 1) & m->mask;
            }
            m->entries[idx] = old_entries[i];
            m->size++;
        }
    }

    free(old_entries);
}

void xmap_set(XMap *m, const char *key, Word value) {
    if (m->size * 4 >= m->capacity * 3) {
        xmap_resize(m, m->capacity * 2);
    }

    uint32_t hash = xmap_hash(key);
    uint32_t idx = hash & m->mask;

    while (m->entries[idx].occupied) {
        if (m->entries[idx].hash == hash && strcmp(m->entries[idx].key, key) == 0) {
            if (m->entries[idx].value.type == WPOINTER &&
                word_owns_memory(&m->entries[idx].value) &&
                m->entries[idx].value.as_pointer != NULL) {
                free(m->entries[idx].value.as_pointer);
            }
            m->entries[idx].value = value;
            return;
        }
        idx = (idx + 1) & m->mask;
    }

    m->entries[idx].key = strdup(key);
    m->entries[idx].value = value;
    m->entries[idx].hash = hash;
    m->entries[idx].occupied = 1;
    m->size++;
}

Word *xmap_get(XMap *m, const char *key) {
    uint32_t hash = xmap_hash(key);
    uint32_t idx = hash & m->mask;
    uint32_t start = idx;

    do {
        if (!m->entries[idx].occupied) {
            return NULL;
        }
        if (m->entries[idx].hash == hash && strcmp(m->entries[idx].key, key) == 0) {
            return &m->entries[idx].value;
        }
        idx = (idx + 1) & m->mask;
    } while (idx != start);

    return NULL;
}

#endif //XMAP_H
