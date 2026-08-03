#include "emalloc.h"

void* malloc(size_t size)
{
    return emalloc(size);
}

void free(void* ptr)
{
    efree(ptr);
}