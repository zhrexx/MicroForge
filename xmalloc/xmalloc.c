#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include "xmalloc.h"

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
    #include <windows.h>
#elif defined(__unix__) || defined(__unix) || defined(unix) || (defined(__APPLE__) && defined(__MACH__))
    #define PLATFORM_UNIX
    #include <unistd.h>
    #include <sys/mman.h>
    #if defined(__APPLE__) && defined(__MACH__)
        #define PLATFORM_APPLE
    #endif
#else
    #error "Unsupported platform"
#endif

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define CHUNK_SIZE (64 * 1024)
#define MIN_ALLOC_SIZE 16
#define BLOCK_MAGIC 0xABCDEF98

typedef struct block_header {
    uint32_t magic;
    size_t size;
    bool is_free;
    struct block_header* next;
    struct block_header* prev;
} block_header_t;

static block_header_t* free_list = NULL;

static void* platform_alloc(size_t size) {
    void* ptr = NULL;
    
#ifdef PLATFORM_WINDOWS
    ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif defined(PLATFORM_UNIX)
    ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED) {
        ptr = NULL;
    }
#endif

    return ptr;
}

static void platform_free(void* ptr, size_t size) {
#ifdef PLATFORM_WINDOWS
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif defined(PLATFORM_UNIX)
    munmap(ptr, size);
#endif
}

static block_header_t* find_free_block(size_t size) {
    block_header_t* current = free_list;
    while (current != NULL) {
        if (current->is_free && current->size >= size) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void split_block(block_header_t* block, size_t size) {
    if (block->size >= size + sizeof(block_header_t) + MIN_ALLOC_SIZE) {
        block_header_t* new_block = (block_header_t*)((char*)block + size);
        new_block->magic = BLOCK_MAGIC;
        new_block->size = block->size - size;
        new_block->is_free = true;
        
        new_block->next = block->next;
        new_block->prev = block;
        if (block->next) {
            block->next->prev = new_block;
        }
        block->next = new_block;
        
        block->size = size;
    }
}

static block_header_t* add_new_chunk(size_t size) {
    size_t alloc_size = (size > CHUNK_SIZE) ? size : CHUNK_SIZE;
    
    block_header_t* new_chunk = platform_alloc(alloc_size);
    if (!new_chunk) {
        return NULL;
    }
    
    new_chunk->magic = BLOCK_MAGIC;
    new_chunk->size = alloc_size;
    new_chunk->is_free = true;
    new_chunk->next = NULL;
    new_chunk->prev = NULL;
    
    if (free_list == NULL) {
        free_list = new_chunk;
    } else {
        block_header_t* current = free_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = new_chunk;
        new_chunk->prev = current;
    }
    
    return new_chunk;
}

static void coalesce(block_header_t* block) {
    if (block->next && block->next->is_free && block->next->magic == BLOCK_MAGIC) {
        block->size += block->next->size;
        block->next = block->next->next;
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    if (block->prev && block->prev->is_free && block->prev->magic == BLOCK_MAGIC) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

static bool is_valid_block(block_header_t* block) {
    if (block->magic != BLOCK_MAGIC) {
        return false;
    }
    
    block_header_t* current = free_list;
    while (current != NULL) {
        if (current == block) {
            return true;
        }
        current = current->next;
    }
    
    return false;
}

void* xmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    size_t aligned_size = ALIGN(size + sizeof(block_header_t));
    if (aligned_size < MIN_ALLOC_SIZE + sizeof(block_header_t)) {
        aligned_size = MIN_ALLOC_SIZE + sizeof(block_header_t);
    }
    
    block_header_t* block = find_free_block(aligned_size);
    
    if (block == NULL) {
        block = add_new_chunk(aligned_size);
        if (block == NULL) {
            return NULL;
        }
    }
    
    split_block(block, aligned_size);
    block->is_free = false;
    
    return (void*)((char*)block + sizeof(block_header_t));
}

void xfree(void* ptr) {
    if (ptr == NULL) {
        return;
    }
    
    block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    
    if (!is_valid_block(block)) {
        return;
    }
    
    if (block->is_free) {
        return;
    }
    
    block->is_free = true;
    coalesce(block);
}

void* xrealloc(void* ptr, size_t size) {
    if (ptr == NULL) {
        return xmalloc(size);
    }
    
    if (size == 0) {
        xfree(ptr);
        return NULL;
    }
    
    block_header_t* block = (block_header_t*)((char*)ptr - sizeof(block_header_t));
    
    if (!is_valid_block(block)) {
        return xmalloc(size);
    }
    
    size_t aligned_size = ALIGN(size + sizeof(block_header_t));
    
    if (block->size >= aligned_size) {
        return ptr;
    }
    
    void* new_ptr = xmalloc(size);
    if (new_ptr == NULL) {
        return NULL;
    }
    
    size_t copy_size = block->size - sizeof(block_header_t);
    if (copy_size > size) {
        copy_size = size;
    }
    
    char* src = (char*)ptr;
    char* dst = (char*)new_ptr;
    for (size_t i = 0; i < copy_size; i++) {
        dst[i] = src[i];
    }
    
    xfree(ptr);
    return new_ptr;
}

void* xcalloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    
    if (size != 0 && total_size / size != nmemb) {
        return NULL;
    }
    
    void* ptr = xmalloc(total_size);
    if (ptr != NULL) {
        char* p = (char*)ptr;
        for (size_t i = 0; i < total_size; i++) {
            p[i] = 0;
        }
    }
    
    return ptr;
}
