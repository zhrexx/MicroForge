#ifndef BB_H
#define BB_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

typedef struct {
    uint8_t *data;
    size_t   len;
    size_t   cap;
} ByteBuf;

static inline void bb_init(ByteBuf *b, size_t cap) {
    b->data = malloc(cap);
    b->len  = 0;
    b->cap  = cap;
}

static inline void bb_free(ByteBuf *b) {
    free(b->data);
    b->data = NULL;
    b->len = b->cap = 0;
}

static inline void bb_clear(ByteBuf *b) {
    b->len = 0;
}

static void bb_reserve(ByteBuf *b, size_t need) {
    if (b->len + need > b->cap) {
        size_t newcap = b->cap ? b->cap * 2 : 16;
        while (newcap < b->len + need)
            newcap *= 2;
        b->data = realloc(b->data, newcap);
        b->cap  = newcap;
    }
}

static inline void bb_push(ByteBuf *b, uint8_t v) {
    bb_reserve(b, 1);
    b->data[b->len++] = v;
}

static inline void bb_append(ByteBuf *b, const void *src, size_t n) {
    bb_reserve(b, n);
    memcpy(b->data + b->len, src, n);
    b->len += n;
}

void bb_appendf(ByteBuf *b, const char *types, ...) {
    va_list ap;
    va_start(ap, types);

    for (const char *p = types; *p; p++) {
        switch (*p) {
            case 'c': {
                int v = va_arg(ap, int);
                uint8_t c = (uint8_t)v;
                bb_append(b, &c, 1);
                break;
            }
            case 'b': {
                unsigned v = va_arg(ap, unsigned);
                uint8_t u = (uint8_t)v;
                bb_append(b, &u, 1);
                break;
            }
            case 'h': {
                unsigned v = va_arg(ap, unsigned);
                uint16_t u = (uint16_t)v;
                bb_append(b, &u, 2);
                break;
            }
            case 'i': {
                int v = va_arg(ap, int);
                bb_append(b, &v, 4);
                break;
            }
            case 'q': {
                long long v = va_arg(ap, long long);
                bb_append(b, &v, 8);
                break;
            }
            case 's': {
                const char *s = va_arg(ap, const char *);
                size_t n = strlen(s);
                bb_append(b, s, n);
                break;
            }
        }
    }

    va_end(ap);
}

#endif