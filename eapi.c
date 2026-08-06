#include "emalloc.h"

void* malloc(size_t size)
{
    return emalloc(size);
}

void* calloc(size_t size, size_t count)
{
    return ecalloc(size, count);
}

void* realloc(void *ptr, size_t size)
{
    return erealloc(ptr, size);
}

void free(void* ptr)
{
    efree(ptr);
}

size_t malloc_usable_size(void* ptr)
{
    return emalloc_usable_size(ptr);
}