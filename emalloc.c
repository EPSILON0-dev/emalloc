#include "emalloc.h"
#include "econfig.h"

void *emalloc(size_t size)
{
    if (size <= SLAB_ALLOCATOR_MAX_OBJECT)
    {
        return slab_alloc(size);
    }

    return NULL;
}

void efree(void *ptr)
{
    (void)ptr;
    // pass
}
