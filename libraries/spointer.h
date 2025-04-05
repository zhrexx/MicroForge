#ifndef SPOINTER_H
#define SPOINTER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

typedef struct {
    size_t size;
    void *ptr;
    bool freed;
    unsigned int ref_count;
} SPointer;

SPointer spalloc(size_t size) {
    SPointer result = {0};
    result.size = size;
    result.ptr = malloc(size);
    result.freed = false;
    result.ref_count = 1;  
    assert(result.ptr != NULL);
    return result;
}

void spfree(SPointer *sp) {
    if (sp->freed || sp->ref_count > 0) return;
    sp->size = 0;
    memset(sp->ptr, 0, sp->size); 
    free(sp->ptr);
    sp->ptr = NULL;
    sp->freed = true;
}

bool spresize(SPointer *sp, size_t new_size) {
    if (sp->freed) return false;
    void *new_ptr = realloc(sp->ptr, new_size);
    if (new_ptr == NULL) return false;
    sp->ptr = new_ptr;
    sp->size = new_size;
    return true;
}

SPointer spcopy(SPointer *sp) {
    SPointer new_sp = {0};
    new_sp.size = sp->size;
    new_sp.ptr = malloc(sp->size);
    if (new_sp.ptr != NULL) {
        memcpy(new_sp.ptr, sp->ptr, sp->size);
        new_sp.freed = false;
        new_sp.ref_count = 1;
    }
    return new_sp;
}

SPointer spmove(SPointer *sp) {
    SPointer new_sp = *sp;
    sp->freed = true;
    sp->ptr = NULL;
    sp->size = 0;
    sp->ref_count = 0;
    return new_sp;
}

void spretain(SPointer *sp) {
    if (!sp->freed) {
        sp->ref_count++;
    }
}

void sprelease(SPointer *sp) {
    if (!sp->freed && sp->ref_count > 0) {
        sp->ref_count--;
        if (sp->ref_count == 0) {
            spfree(sp);
        }
    }
}

SPointer spaligned_alloc(size_t size, size_t alignment) {
    SPointer result = {0};
    result.size = size;
    result.ptr = aligned_alloc(alignment, size);
    result.freed = false;
    result.ref_count = 1;
    assert(result.ptr != NULL);
    return result;
}


#endif

