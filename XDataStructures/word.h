#ifndef XWORD_H
#define XWORD_H

#include <stdint.h>
#include <string.h>

typedef enum {
    WINT,
    WFLOAT,
    WPOINTER,
    WCHAR_,
    WUINT64,
} WordType;

typedef struct {
    WordType type;
    bool owns_memory;
    union {
        void *as_pointer;
        int as_int;
        uint64_t as_uint64;
        double as_float;
        char as_char;
    };
} Word;

static inline Word word_int(int value) {
    Word word;
    word.type = WINT;
    word.as_int = value;
    return word;
}

static inline Word word_uint64(uint64_t value) {
    Word word;
    word.type = WUINT64;
    word.as_uint64 = value;
    return word;
}

static inline Word word_float(double value) {
    Word word;
    word.type = WFLOAT;
    word.as_float = value;
    return word;
}

static inline Word word_pointer(void *value) {
    Word word;
    word.type = WPOINTER;
    word.as_pointer = value;
    word.owns_memory = false;
    return word;
}

static inline Word word_char(char value) {
    Word word;
    word.type = WCHAR_;
    word.as_char = value;
    return word;
}

static inline Word word_string(const char *value) {
    Word word;
    word.type = WPOINTER;
    word.as_pointer = (void*)strdup(value);
    word.owns_memory = true;
    return word;
}

#define WORD(val) _Generic((val), \
int: word_int, \
long: word_int, \
long long: word_uint64, \
unsigned int: word_uint64, \
unsigned long: word_uint64, \
unsigned long long: word_uint64, \
float: word_float, \
double: word_float, \
char: word_char, \
char*: word_string, \
const char*: word_string, \
void*: word_pointer, \
default: word_pointer \
)(val)

#endif //XWORD_H