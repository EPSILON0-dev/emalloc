#include "emalloc.h"

void panic(const char *error)
{
    write(2, error, strlen(error));
    abort();
}

#if defined(DEBUG_LOG_CALLS) || defined(DEBUG_LOG_HEAP_EXTENSIONS)
static char dig_to_hex(int dig)
{
    return (dig < 10) ? dig + '0' : dig + 'a' - 10;
}
#endif

#ifdef DEBUG_LOG_CALLS
void log_malloc_call_start(size_t size)
{
    static char buffer[64] = "malloc(0x00000000)";
    const size_t size_offset = 9;

    for (int i = 0; i < 8; i++)
    {
        int dig = (size >> 28) & 0xf;
        buffer[i + size_offset] = dig_to_hex(dig);
        size <<= 4;
    }

    write(2, buffer, strlen(buffer));
}

void log_malloc_call_end(void *ptr)
{
    static char buffer[64] = " --> 0x0000000000000000\n";
    const size_t location_offset = 7;

    uintptr_t ptr_val = (uintptr_t)ptr;
    for (int i = 0; i < 16; i++)
    {
        int dig = (ptr_val >> 60) & 0xf;
        buffer[i + location_offset] = dig_to_hex(dig);
        ptr_val <<= 4;
    }

    write(2, buffer, strlen(buffer));
}

void log_free_call(void *ptr)
{
    static char buffer[64] = "free(0x0000000000000000)\n";
    const size_t location_offset = 7;

    uintptr_t ptr_val = (uintptr_t)ptr;
    for (int i = 0; i < 16; i++)
    {
        int dig = (ptr_val >> 60) & 0xf;
        buffer[i + location_offset] = dig_to_hex(dig);
        ptr_val <<= 4;
    }

    write(2, buffer, strlen(buffer));
}
#endif

#ifdef DEBUG_LOG_HEAP_EXTENSIONS
void log_heap_extension(void *ptr)
{
    static char buffer[64] = "brk(0x0000000000000000)\n";
    const size_t location_offset = 6;

    uintptr_t ptr_val = (uintptr_t)ptr;
    for (int i = 0; i < 16; i++)
    {
        int dig = (ptr_val >> 60) & 0xf;
        buffer[i + location_offset] = dig_to_hex(dig);
        ptr_val <<= 4;
    }

    write(2, buffer, strlen(buffer));
}
#endif