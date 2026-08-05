#include "emalloc.h"

spinlock_t global_lock = SPINLOCK_INIT;

void* emalloc(size_t size)
{
    spin_lock(&global_lock);

    void* ptr = NULL;

    if (size <= (1 << SLAB_ALLOCATOR_MAX_OBJECT))
    {
        ptr = slab_alloc(size);
    }
    else if (size <= (1 << BUDDY_ALLOCATOR_MAX_OBJECT))
    {
        ptr = buddy_alloc(size);
    }
    else
    {
        ptr = mmap_alloc(size);
    }

    spin_unlock(&global_lock);

#ifdef DEBUG_LOG_CALLS
    log_malloc_call(ptr, size);
#endif

    return ptr;
}

void efree(void* ptr)
{
    spin_lock(&global_lock);

    const void* heap_end = brk_get_allocated_heap_end();

    if (ptr > heap_end)
    {
        mmap_free(ptr);
    }
    else
    {
        // Slab free is routed through the buddy allocator's free
        buddy_free(ptr);
    }

    spin_unlock(&global_lock);

#ifdef DEBUG_LOG_CALLS
    log_free_call(ptr);
#endif
}
