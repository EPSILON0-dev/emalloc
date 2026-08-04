#include "emalloc.h"
#include "econfig.h"

void *emalloc(size_t size)
{
    void* ptr = NULL;

    if (size <= (1 << SLAB_ALLOCATOR_MAX_OBJECT))
    {
        ptr = slab_alloc(size);
    }
    else if (size <= (1 << BUDDY_ALLOCATOR_MAX_OBJECT))
    {
        ptr = buddy_alloc(size);
    }

    return ptr;
}

void efree(void *ptr)
{
    // Free is always routed through the buddy allocator's free
    buddy_free(ptr);
}
