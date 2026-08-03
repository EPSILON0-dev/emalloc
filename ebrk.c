#include <unistd.h>

#include "econfig.h"
#include "emalloc.h"

static struct
{
    size_t current_size;
    size_t current_used;
    size_t current_free;
    void* heap_end;
    void* allocated_heap_end;
} heap_state;

static __attribute__((constructor)) void brk_manager_init(void)
{
    heap_state.current_size = 0;
    heap_state.current_used = 0;
    heap_state.current_free = 0;
    heap_state.heap_end = sbrk(0);
    heap_state.allocated_heap_end = heap_state.heap_end;
}

void* brk_alloc(size_t size)
{
    // Allocate more space from the kernel if needed
    while (size > heap_state.current_free)
    {
        const size_t increment = (1 << BRK_ENLARGE_INCREMENT);
        heap_state.heap_end = sbrk(increment);
        heap_state.current_size += increment;
        heap_state.current_free += increment;
    }

    // Get the current end of brk
    void *alloc_ptr = heap_state.allocated_heap_end;

    // Allocate in the userspace brk
    heap_state.current_used += size;
    heap_state.current_free -= size;
    heap_state.allocated_heap_end = (void*)((uintptr_t)heap_state.allocated_heap_end + size);

    // Return the address of the allocation
    return alloc_ptr;
}