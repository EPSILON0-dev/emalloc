#include <unistd.h>

#include "econfig.h"
#include "emalloc.h"

static void* heap_end;

static void* align_heap(void)
{
    void* curr_brk_ptr = sbrk(0);
    void* aligned_ptr = curr_brk_ptr;

    const size_t arena_size = (1ULL << BUDDY_ALLOCATOR_ARENA_SIZE);
    const uintptr_t curr_brk = (uintptr_t)curr_brk_ptr;
    const uintptr_t aligned_brk = curr_brk & ~((1ULL << BUDDY_ALLOCATOR_ARENA_SIZE) - 1);

    if (aligned_brk != curr_brk)
    {
        const uintptr_t next_aligned = aligned_brk + arena_size;
        const uintptr_t dummy_size = next_aligned - curr_brk;
        if ((uintptr_t)sbrk(dummy_size) == (uintptr_t)(-1))
        {
            panic("heap: heap alignment failed");
        }
        aligned_ptr = (void*)(curr_brk + dummy_size);
    }

    return aligned_ptr;
}

void* brk_allocate_buddy_arena()
{
    const size_t arena_size = (1 << BUDDY_ALLOCATOR_ARENA_SIZE);
    void* alloc_ptr = align_heap();

    const void* old_heap_end = sbrk(arena_size);
    if ((uintptr_t)old_heap_end == (uintptr_t)-1)
    {
        panic("heap: extension failed\n");
    }
    heap_end = (void*)((uintptr_t)old_heap_end + arena_size);

#ifdef DEBUG_LOG_HEAP_EXTENSIONS
    log_heap_extension(heap_state.heap_end);
#endif

    return alloc_ptr;
}

void* brk_get_allocated_heap_end(void)
{
    return heap_end;
}