#ifndef SB_H
#define SB_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} StringBuilder;

StringBuilder sb_init(size_t capacity) {
    if (capacity < 1) return (StringBuilder){0};
    StringBuilder sb = {0};
    sb.data = (char *)malloc(capacity);
    sb.capacity = capacity;
    sb.size = 0;
    return sb;
}

void sb_destroy(StringBuilder *sb) {
    if (sb->data == NULL) return;
    free(sb->data);
    sb->data = NULL;
    sb->size = 0;
    sb->capacity = 0;
}

void sb_resize(StringBuilder *sb, size_t new_capacity) {
    sb->capacity = new_capacity;
    sb->data = (char *)realloc(sb->data, new_capacity);
    assert(sb->data != NULL);
}

char sb_append(StringBuilder *sb, char ch) {
    if ((sb->size + 1) > sb->capacity) {
        sb_resize(sb, sb->capacity ? sb->capacity * 2 : 1);
    }
    sb->data[sb->size++] = ch;
    return ch;
}

char sb_pop(StringBuilder *sb) {
    if (sb->size < 1) assert(false && "Invalid size");
    return sb->data[--sb->size];
}

int sb_append_str(StringBuilder *sb, const char *str) {
    if (!str) return 0;
    size_t previous_size = sb->size;
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        sb_append(sb, str[i]);
    }
    return sb->size - previous_size;
}

int sb_remove(StringBuilder *sb, size_t chars) {
    if (chars > sb->size) {
        chars = sb->size;
    }
    
    for (size_t i = 0; i < chars; i++) {
        sb_pop(sb);
    }
    return chars;
}

char *sb_to_cstr(StringBuilder *sb) {
    char *str = malloc(sb->size+1);
    size_t str_cur = 0;
    for (size_t i = 0; i < sb->size; i++) {
        str[str_cur++] = sb_pop(sb);
    }
    str[str_cur] = '\0';
    return str;
}

StringBuilder sb_slice(StringBuilder *sb, size_t chars) {
    if (chars > sb->size) {
        chars = sb->size;
    }
    StringBuilder result = sb_init(chars);
    memcpy(result.data, sb->data, chars);
    result.size = chars;
    return result;
}

void sb_reset(StringBuilder *sb) {
    sb->size = 0;
}

#endif

