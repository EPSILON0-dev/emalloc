#include "emalloc.h"

#include <string.h>

spinlock_t global_lock = SPINLOCK_INIT;

void* emalloc(size_t size)
{
#ifdef DEBUG_LOG_CALLS
    log_malloc_call_start(size);
#endif

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
    log_malloc_call_end(ptr);
#endif

    return ptr;
}

void* ecalloc(size_t count, size_t size)
{
    if (size != 0 && count > SIZE_MAX / size)
    {
        return NULL;
    }

    const size_t actual_size = count * size;
    void* ptr = emalloc(actual_size);

    if (ptr != NULL)
    {
        memset(ptr, 0, actual_size);
    }

    return ptr;
}

void* erealloc(void* ptr, size_t size)
{
    if (ptr == NULL)
    {
        return emalloc(size);
    }

    if (size == 0)
    {
        efree(ptr);
        return NULL;
    }

    const size_t old_size = emalloc_usable_size(ptr);
    void* new_ptr = emalloc(size);

    const size_t copy_size = (size < old_size) ? size : old_size;
    memcpy(new_ptr, ptr, copy_size);

    efree(ptr);
    return new_ptr;
}

void efree(void* ptr)
{
    if (ptr == NULL)
    {
        return;
    }

#ifdef DEBUG_LOG_CALLS
    log_free_call(ptr);
#endif

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
}

size_t emalloc_usable_size(void* ptr)
{
    spin_lock(&global_lock);

    const void* heap_end = brk_get_allocated_heap_end();
    size_t size;

    if (ptr > heap_end)
    {
        size = mmap_usable_size(ptr);
    }
    else
    {
        // Slab get_realloc_size is routed through the buddy allocator
        size = buddy_usable_size(ptr);
    }

    spin_unlock(&global_lock);
    return size;
}
