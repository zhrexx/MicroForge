#ifndef XVEC_H
#define XVEC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef enum {
    WINT,
    WFLOAT,
    WPOINTER,
    WCHAR_,
} WordType;

typedef struct {
    WordType type; 
    union {
        void *as_pointer;
        int as_int;
        float as_float;
        char as_char;
    };
} Word;

typedef struct {
    Word *data;
    size_t size;
    size_t capacity;
} XVec;

Word word_int(int value);
Word word_float(float value);
Word word_pointer(void *value);
Word word_char(char value);
Word word_string(const char *value);

void xvec_init(XVec *vector, size_t initial_capacity);
void xvec_free(XVec *vector);
XVec xvec_create(size_t initial_capacity);
void xvec_resize(XVec *vector, size_t new_capacity);
void xvec_push(XVec *vector, Word value);
Word xvec_pop(XVec *vector);
Word *xvec_get(XVec *vector, size_t index);
void xvec_set(XVec *vector, size_t index, Word value);
void xvec_remove(XVec *vector, size_t index);
ssize_t xvec_find(XVec *vector, Word value);
bool xvec_contains(XVec *vector, Word value);
void xvec_compress(XVec *vector);
void xvec_copy(XVec *dest, const XVec *src);
size_t xvec_len(XVec *vector);
bool xvec_empty(XVec *vector);

#define XVEC_FOR_EACH(element, vector) \
    for (Word *element = (vector)->data; \
         element < (vector)->data + (vector)->size; \
         element++)

char *xvec_to_string(XVec *vector, const char *separator);
XVec parse_pargs(int argc, char **argv);
XVec split_to_vector(const char* src, const char* delimiter);

#endif
