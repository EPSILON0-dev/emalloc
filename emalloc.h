#ifndef EMALLOC_H
#define EMALLOC_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdatomic.h>

#include "econfig.h"

enum EBitmapSlot
{
    E_BIT_FREE = 0,
    E_BIT_USED = 1,
};

enum EResult
{
    E_RESULT_OK = 0,
    E_RESULT_ERROR = 1,
    E_RESULT_SLAB_ALLOCATION = 2,
};

typedef void slab_t;

struct slab_chain_head
{
    uint16_t object_size;

    uint32_t slab_count;
    uint32_t first_free_index;

    slab_t* first_free_slab;
    slab_t* first_slab;
};

typedef struct slab_chain_head slab_chain_head_t;

struct slab_header
{
    uint32_t magic1;

    // Size and count of each object in this slab (count includes the objects used by the header)
    uint16_t object_size;
    uint16_t object_count;

    // Number of free objects in this slab
    uint16_t free_object_count;

    // First object that is not a header
    uint16_t first_usable_index;

    // First free object (refilled after a free or allocation)
    uint16_t first_free_index;

    // Bookkeeping info for the slabs
    uint32_t index_in_chain;
    slab_chain_head_t* head;
    slab_t* next_slab;

    uint32_t magic2;

    // Bitmap of free objects
    uint8_t bitmap[];
};

typedef struct slab_header slab_header_t;

typedef void buddy_arena_t;

struct buddy_chain_head
{
    uint32_t arena_count;
    uint16_t first_free_of_size_index[BUDDY_ALLOCATOR_SIZES];
    buddy_arena_t* head_arena;
    buddy_arena_t* tail_arena;
    buddy_arena_t* first_free_of_size[BUDDY_ALLOCATOR_SIZES];
};

typedef struct buddy_chain_head buddy_chain_head_t;

struct buddy_header
{
    uint32_t magic1;

    // Bookkeeping for the arena chain
    uint32_t index_in_chain;
    buddy_arena_t* next_arena;

    // Free slot counts
    uint16_t free_slot_count_of_size[BUDDY_ALLOCATOR_SIZES];

    // Bitmap of slots used by the slabs
    uint8_t slab_bitmap[BUDDY_SLAB_BITMAP_BYTES];

    uint16_t magic2;

    // Bitmaps of all used slots for each slot size
    // Stacked one after the other
    // (512 bits - 1 slot), (256 bits - 2 slots), (128 bits - 4 slots), ( ... )
    uint8_t slot_bitmap[BUDDY_SLOT_BITMAP_BYTES];
};

typedef struct buddy_header buddy_header_t;

struct mmap_header
{
    uint32_t magic1;
    void* allocation_address;
    size_t allocation_length;
    uint32_t magic2;
};

typedef struct mmap_header mmap_header_t;

struct spinlock
{
    atomic_flag locked;
};

typedef struct spinlock spinlock_t;

#define SPINLOCK_INIT { ATOMIC_FLAG_INIT }

void panic(const char* error);

void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);

void* brk_alloc(size_t size);
void *brk_get_allocated_heap_end(void);

void* buddy_alloc(size_t size);
void* buddy_alloc_slab(size_t size);
void buddy_free(void* ptr);
size_t buddy_get_realloc_size(void* ptr);

void* slab_alloc(size_t size);
void slab_free(void* ptr);
size_t slab_get_realloc_size(void* ptr);

void* mmap_alloc(size_t size);
void mmap_free(void* ptr);
size_t mmap_get_realloc_size(void* ptr);

size_t get_realloc_size(void* ptr);

void* emalloc(size_t size);
void* ecalloc(size_t count, size_t size);
void* erealloc(void *ptr, size_t size);
void efree(void* ptr);

void log_malloc_call_start(size_t size);
void log_malloc_call_end(void *ptr);
void log_free_call(void *ptr);
void log_heap_extension(void *ptr);
void log_heap_tamper(void *old_ptr, void* new_ptr);

#endif