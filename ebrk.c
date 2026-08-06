#include <unistd.h>

#include "econfig.h"
#include "emalloc.h"

static struct
{
    size_t space_left;
    void* prev_heap_end;
    void* alloc_ptr;
    void* heap_end;
} heap_state = {};

static void* align_heap(void)
{
    void* aligned_ptr = heap_state.alloc_ptr;

    void* curr_brk_ptr = sbrk(0);
    uintptr_t curr_brk = (uintptr_t)curr_brk_ptr;

    // If tampering was detected, reset the space_left and alloc_ptr
    if (heap_state.prev_heap_end != NULL && heap_state.prev_heap_end != curr_brk_ptr)
    {
#ifdef DEBUG_LOG_HEAP_TAMPERING
        log_heap_tamper(heap_state.prev_heap_end, curr_brk_ptr);
#endif
        heap_state.space_left = 0;
        heap_state.alloc_ptr = NULL;
    }

    // Allocate dummy space to align the brk
    const uintptr_t aligned_brk = curr_brk & ~((1ULL << BUDDY_ALLOCATOR_ARENA_SIZE) - 1);
    if (aligned_brk != curr_brk)
    {
        const uintptr_t next_aligned = aligned_brk + (1ULL << BUDDY_ALLOCATOR_ARENA_SIZE);
        const uintptr_t dummy_size = next_aligned - curr_brk;
        if ((uintptr_t)sbrk(dummy_size) == (uintptr_t)(-1))
        {
            panic("heap: heap alignment failed");
        }
        aligned_ptr = (void*)(curr_brk + dummy_size);
    }

    return aligned_ptr;
}

static bool verify_buddy_arena_ptr_alignment(void* ptr)
{
    void* aligned_ptr = (void*)((uintptr_t)ptr & ~((1ULL << BUDDY_ALLOCATOR_ARENA_SIZE) - 1));
    return aligned_ptr == ptr;
}

void* brk_alloc(size_t size)
{
    void* alloc_ptr = NULL;

    // Allocate more space from the kernel if needed
    while (size > heap_state.space_left)
    {
        alloc_ptr = align_heap();
        const size_t increment = (1 << BRK_ENLARGE_INCREMENT);
        const void* old_heap_end = sbrk(increment);
        heap_state.space_left += increment;
        heap_state.alloc_ptr = alloc_ptr;

        if ((uintptr_t)old_heap_end == (uintptr_t)-1)
        {
            panic("heap: extension failed\n");
        }

        heap_state.heap_end = (void*)((uintptr_t)old_heap_end + increment);
    }

    // Might not be the best way to handle this
    if (!verify_buddy_arena_ptr_alignment(heap_state.alloc_ptr))
    {
        panic("heap: misalignment detected\n");
    }

    // Allocate from the heap we already have
    alloc_ptr = heap_state.alloc_ptr;
    heap_state.alloc_ptr = (void*)((uintptr_t)heap_state.alloc_ptr + size);
    heap_state.space_left -= size;

#ifdef DEBUG_LOG_HEAP_EXTENSIONS
    log_heap_extension(heap_state.heap_end);
#endif

    // Return the address of the allocation
    return alloc_ptr;
}

void* brk_get_allocated_heap_end(void)
{
    return heap_state.heap_end;
}