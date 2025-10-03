//
// Created by dev on 10/1/25.
//

#ifndef XMAP_H
#define XMAP_H
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "word.h"

typedef struct {
    const char* key;
    Word value;
} XEntry;

typedef struct {
    XEntry *entries;
    size_t size;
    size_t capacity;
} XMap;

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void xmap_init(XMap *m, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 1;
    m->entries = malloc(initial_capacity * sizeof(XEntry));
    if (!m->entries) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    m->size = 0;
    m->capacity = initial_capacity;
}

void xmap_free(XMap *m) {
    for (size_t i = 0; i < m->size; i++) {
        if (m->entries[i].value.type == WPOINTER && m->entries[i].value.owns_memory && m->entries[i].value.as_pointer != NULL) {
            free(m->entries[i].value.as_pointer);
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
    if (new_capacity == 0) new_capacity = 1;
    XEntry *new_entries = realloc(m->entries, new_capacity * sizeof(XEntry));
    if (!new_entries) {
        fprintf(stderr, "Failed to reallocate memory\n");
        exit(EXIT_FAILURE);
    }
    m->entries = new_entries;
    m->capacity = new_capacity;
}

void xmap_set(XMap *m, const char *key, Word value) {
    for (size_t i = 0; i < m->size; i++) {
        if (strcmp(m->entries[i].key, key) == 0) {
            if (m->entries[i].value.type == WPOINTER && m->entries[i].value.owns_memory && m->entries[i].value.as_pointer != NULL) {
                free(m->entries[i].value.as_pointer);
            }
            m->entries[i].value = value;
            return;
        }
    }

    if (m->size >= m->capacity) {
        xmap_resize(m, m->capacity * 2);
    }

    m->entries[m->size].key = strdup(key);
    m->entries[m->size].value = value;
    m->size++;
}

Word *xmap_get(XMap *m, const char *key) {
    for (size_t i = 0; i < m->size; i++) {
        if (strcmp(m->entries[i].key, key) == 0) {
            return &m->entries[i].value;
        }
    }
    return NULL;
}

#endif //XMAP_H