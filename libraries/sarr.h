// String Array
#ifndef SARR_H
#define SARR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct {
    char **strings;
    size_t capacity;
    size_t size;
} StringArray;

StringArray sarr_create(size_t capacity) {
    StringArray result = {0};
    result.strings = (char **)malloc(capacity * sizeof(char *));
    result.capacity = capacity;
    result.size = 0;
    assert(result.strings != NULL);
    return result;
}

void sarr_free(StringArray *sarr) {
    if (sarr->strings == NULL) return;
    for (size_t i = 0; i < sarr->size; i++) {
        free(sarr->strings[i]);
    }
    free(sarr->strings);

    sarr->strings = NULL;
    sarr->capacity = 0;
    sarr->size = 0;
}

void sarr_resize(StringArray *sarr, size_t new_capacity) {
    sarr->capacity = new_capacity;
    sarr->strings = (char **)realloc(sarr->strings, new_capacity*sizeof(char*));
    assert(sarr->strings != NULL);
}

void sarr_append(StringArray *sarr, char *string) {
    if (!string) return;
    if ((sarr->size + 1) > sarr->capacity) {
        sarr_resize(sarr, sarr->capacity ? sarr->capacity * 2 : 1);
    }
    sarr->strings[sarr->size++] = strdup(string); 
}

char *sarr_get(StringArray *sarr, size_t index) {
    assert(index < sarr->size);
    return sarr->strings[index];
}

char *sarr_get_copy(StringArray *sarr, size_t index) {
    assert(index < sarr->size);
    return strdup(sarr->strings[index]);
}

char *sarr_pop(StringArray *sarr) {
    assert(sarr->size != 0);
    char *result = sarr->strings[--sarr->size];
    sarr->strings[sarr->size] = NULL;
    return result;
}

void sarr_remove(StringArray *sarr, size_t index) {
    assert(index < sarr->size);
    free(sarr->strings[index]);
    for (size_t i = index+1; i < sarr->size; i++) {
       sarr->strings[i-1] = sarr->strings[i]; 
    }
    sarr->size--;
}

char *sarr_combine(StringArray *sarr) {
    size_t result_size = 0;
    for (size_t i = 0; i < sarr->size; i++) {
        result_size += strlen(sarr->strings[i]);
    }

    if (result_size == 0) return strdup("");

    char *result = malloc(result_size + 1);
    assert(result != NULL);

    size_t cursor = 0;
    for (size_t i = 0; i < sarr->size; i++) {
        size_t len = strlen(sarr->strings[i]);
        memcpy(result + cursor, sarr->strings[i], len);
        cursor += len;
    }

    result[cursor] = '\0';
    return result;
}

StringArray str_to_sarr(char *original_str, char *delim) {
    StringArray sarr = sarr_create(10);
    char *str = strdup(original_str);
    assert(str != NULL);
    char *token = strtok(str, delim);
    while (token != NULL) {
        sarr_append(&sarr, token);
        token = strtok(NULL, delim);
    }
    free(str);
    return sarr;
}


#endif // SARR_H
