#ifndef XMALLOC_H
#define XMALLOC_H

#include <stddef.h>

/**
 * Allocates memory of the specified size.
 *
 * @param size The number of bytes to allocate
 * @return A pointer to the allocated memory, or NULL if allocation fails
 */
void* xmalloc(size_t size);

/**
 * Frees memory previously allocated with xmalloc, xrealloc, or xcalloc.
 *
 * @param ptr Pointer to memory to free. If NULL, no operation is performed.
 */
void xfree(void* ptr);

/**
 * Changes the size of a previously allocated memory block.
 *
 * @param ptr Pointer to memory previously allocated. If NULL, equivalent to xmalloc(size).
 * @param size The new size in bytes. If 0 and ptr is not NULL, equivalent to xfree(ptr).
 * @return A pointer to the reallocated memory, or NULL if allocation fails
 */
void* xrealloc(void* ptr, size_t size);

/**
 * Allocates memory for an array of elements and initializes all bytes to zero.
 *
 * @param nmemb Number of elements to allocate
 * @param size Size of each element in bytes
 * @return A pointer to the allocated memory initialized to zero, or NULL if allocation fails
 */
void* xcalloc(size_t nmemb, size_t size);

#endif /* XMALLOC_H */
