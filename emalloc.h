#ifndef EMALLOC_H
#define EMALLOC_H

#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

struct slab_header
{
    // Size of the slab (memory allocated for objects)
    uint32_t slab_size;

    // Size and count of each object in this slab
    uint16_t object_size;
    uint16_t object_count;

    // Number of free objects in this slab
    uint16_t free_object_count;

    // First object that is not a header
    uint16_t first_usable_index;

    // First free object (refilled after a free or allocation)
    uint16_t first_free_index;

    // Pointer to the next slab
    void* next_slab;
    
    // Bitmap of free objects
    uint8_t bitmap[];
};

typedef struct slab_header slab_header_t;

struct slab_chain_head
{
    uint32_t slab_size;
    uint16_t object_size;
    void *first_free_slab;
    void *first_slab;
};

typedef struct slab_chain_head slab_chain_head_t;

typedef void slab_t;

void panic(const char *error);

void *brk_alloc(size_t size);

void* slab_alloc(size_t size);

void* emalloc(size_t size);
void efree(void* ptr);

#endif