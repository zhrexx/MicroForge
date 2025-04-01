#ifndef VECTOR_H
#define VECTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    void *data;
    size_t size;
    size_t capacity;
    size_t element_size;
} Vector;

void vector_init(Vector *vector, size_t initial_capacity, size_t element_size);
void vector_resize(Vector *vector, size_t new_capacity);
void vector_push(Vector *vector, const void *value);
void *vector_get(Vector *vector, size_t index);
void vector_set(Vector *vector, size_t index, const void *value);
bool vector_contains(Vector *vector, const void *value);
void vector_remove(Vector *vector, size_t index);
ssize_t vector_find(Vector *vector, const void *value);
void vector_compress(Vector *vector);
void vector_copy(Vector *dest, const Vector *src);
void vector_free(Vector *vector);
size_t vector_len(Vector *vector);
void *vector_pop(Vector *vector);

#define VECTOR_FOR_EACH(type, element, vector) \
    for (type *element = (type *)(vector)->data; \
         (char *)element < (char *)(vector)->data + (vector)->size * (vector)->element_size; \
         element++)

Vector parse_pargs(int argc, char **argv);
Vector split_to_vector(const char* src, const char* delimiter);
char *vector_get_str(Vector *vector, size_t index);
int vector_get_int(Vector *vector, size_t index);
bool vector_empty(Vector *vector);
char *vector_to_string(Vector *vector, const char *separator);
#endif
