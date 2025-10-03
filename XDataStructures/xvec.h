// standalone means its header-only library version

// Why XVec looks like my vector.h
// the code base is shared but this appoch is more user-friendly and modern (Using Word instead of void *)
// i hope you will like it
#ifndef XVEC_H
#define XVEC_H

// TODOS:
// Maybe add WSTRING (cuz of xvec_to_string)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "word.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
    Word *data;
    size_t size;
    size_t capacity;
} XVec;


// Bare XVec creation prefer using xvec_create(size_t initial_capacity);
void xvec_init(XVec *vector, size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 1;
    vector->data = malloc(initial_capacity * sizeof(Word));
    if (!vector->data) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }
    vector->size = 0;
    vector->capacity = initial_capacity;
}

void xvec_free(XVec *vector) {
    for (size_t i = 0; i < vector->size; i++) {
        if (vector->data[i].type == WPOINTER && vector->data[i].owns_memory &&
            vector->data[i].as_pointer != NULL) {
            free(vector->data[i].as_pointer);
        }
    }

    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

///////////////////////////////////////
/// User API
///////////////////////////////////////

XVec xvec_create(size_t initial_capacity) {
    XVec result = {0};
    xvec_init(&result, initial_capacity);
    return result;
}

void xvec_resize(XVec *vector, size_t new_capacity) {
    if (new_capacity == 0) new_capacity = 1;
    Word *new_data = realloc(vector->data, new_capacity * sizeof(Word));
    if (!new_data) {
        fprintf(stderr, "Failed to reallocate memory\n");
        exit(EXIT_FAILURE);
    }
    vector->data = new_data;
    vector->capacity = new_capacity;
}

void xvec_push(XVec *vector, Word value) {
    if (vector->size == vector->capacity) {
        xvec_resize(vector, vector->capacity == 0 ? 1 : vector->capacity * 2);
    }
    vector->data[vector->size] = value;
    vector->size++;
}

Word xvec_pop(XVec *vector) {
    if (vector->size == 0) {
        fprintf(stderr, "Vector is empty\n");
        exit(EXIT_FAILURE);
    }

    Word last_element = vector->data[vector->size - 1];
    vector->size--;
    return last_element;
}


Word *xvec_get(XVec *vector, size_t index) {
    if (index >= vector->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    return &vector->data[index];
}

void xvec_set(XVec *vector, size_t index, Word value) {
    if (index >= vector->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }
    vector->data[index] = value;
}

void xvec_remove(XVec *vector, size_t index) {
    if (index >= vector->size) {
        fprintf(stderr, "Index out of bounds\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = index; i < vector->size - 1; i++) {
        vector->data[i] = vector->data[i + 1];
    }
    vector->size--;
}

//////////////////////////////////
/// Search Operations
//////////////////////////////////

ssize_t xvec_find(XVec *vector, Word value) {
    for (size_t i = 0; i < vector->size; i++) {
        Word current = vector->data[i];

        if (current.type != value.type) {
            continue;
        }

        switch (current.type) {
            case WINT:
                if (current.as_int == value.as_int) return i;
                break;
            case WFLOAT:
                if (current.as_float == value.as_float) return i;
                break;
            case WCHAR_:
                if (current.as_char == value.as_char) return i;
                break;
            case WPOINTER:
                if (current.as_pointer == value.as_pointer) return i;
                break;
        }
    }
    return -1;
}

bool xvec_contains(XVec *vector, Word value) {
    for (size_t i = 0; i < vector->size; i++) {
        Word current = vector->data[i];

        if (current.type != value.type) {
            continue;
        }

        switch (current.type) {
            case WINT:
                if (current.as_int == value.as_int) return true;
                break;
            case WFLOAT:
                if (current.as_float == value.as_float) return true;
                break;
            case WCHAR_:
                if (current.as_char == value.as_char) return true;
                break;
            case WPOINTER:
                if (current.as_pointer == value.as_pointer) return true;
                break;
        }
    }
    return false;
}

/// Utility functions ////////////////////////////////////////////////////////////

void xvec_compress(XVec *vector) {
    if (vector->capacity > vector->size) {
        xvec_resize(vector, vector->size);
    }
}

void xvec_copy(XVec *dest, const XVec *src) {
    xvec_init(dest, src->capacity);
    memcpy(dest->data, src->data, src->size * sizeof(Word));
    dest->size = src->size;
}

size_t xvec_len(XVec *vector) {
    return vector->size;
}

bool xvec_empty(XVec *vector) {
    return vector->size == 0;
}

#define XVEC_FOR_EACH(element, vector) \
    for (Word *element = (vector)->data; \
         element < (vector)->data + (vector)->size; \
         element++)

///////////////////////////////////////
/// useful function
///////////////////////////////////////

char *xvec_to_string(XVec *vector, const char *separator) {
    if (xvec_empty(vector)) {
        return strdup("");
    }

    size_t total_length = 0;
    size_t sep_len = strlen(separator);

    for (size_t i = 0; i < vector->size; i++) {
        if (vector->data[i].type != WPOINTER) {
            fprintf(stderr, "Cannot convert non-string elements to string\n");
            exit(EXIT_FAILURE);
        }
        char *element = (char *)vector->data[i].as_pointer;
        total_length += strlen(element);
    }
    total_length += sep_len * (vector->size - 1);

    char *result = malloc(total_length + 1);
    if (!result) {
        fprintf(stderr, "Failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    char *pos = result;
    for (size_t i = 0; i < vector->size; i++) {
        char *element = (char *)vector->data[i].as_pointer;
        size_t element_len = strlen(element);
        memcpy(pos, element, element_len);
        pos += element_len;
        if (i < vector->size - 1) {
            memcpy(pos, separator, sep_len);
            pos += sep_len;
        }
    }
    *pos = '\0';

    return result;
}

XVec parse_pargs(int argc, char **argv) {
    XVec pargs_vector;
    xvec_init(&pargs_vector, argc);

    for (int i = 0; i < argc; i++) {
        Word word;
        word.type = WPOINTER;
        word.as_pointer = strdup(argv[i]);
        xvec_push(&pargs_vector, word);
    }

    return pargs_vector;
}

XVec split_to_vector(const char* src, const char* delimiter) {
    char* src_copy = strdup(src);
    XVec result;
    xvec_init(&result, 10);

    int in_quote = 0;
    char* start = src_copy;
    char* current = src_copy;
    int delimiter_len = strlen(delimiter);

    while (*current) {
        if (*current == '"') {
            in_quote = !in_quote;
        } else if (!in_quote && strncmp(current, delimiter, delimiter_len) == 0) {
            *current = '\0';
            if (start != current) {
                Word word;
                word.type = WPOINTER;
                word.as_pointer = strdup(start);
                xvec_push(&result, word);
            }
            start = current + delimiter_len;
            current += delimiter_len - 1;
        }
        current++;
    }

    if (start != current) {
        Word word;
        word.type = WPOINTER;
        word.as_pointer = strdup(start);
        xvec_push(&result, word);
    }

    free(src_copy);
    return result;
}


#endif