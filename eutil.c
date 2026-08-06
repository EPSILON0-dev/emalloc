#include "emalloc.h"

void panic(const char* error)
{
    write(2, error, strlen(error));
    abort();
}

#if defined(DEBUG_LOG_CALLS) || defined(DEBUG_LOG_HEAP_EXTENSIONS) || \
    defined(DEBUG_LOG_HEAP_TAMPERING)
static char dig_to_hex(int dig)
{
    return (dig < 10) ? dig + '0' : dig + 'a' - 10;
}

__attribute__((unused)) static void print_hex_32(char* ptr, uint32_t value)
{
    for (int i = 0; i < 8; i++)
    {
        int dig = (value >> 28) & 0xf;
        ptr[i] = dig_to_hex(dig);
        value <<= 4;
    }
}

static void print_hex_64(char* ptr, uint64_t value)
{
    for (int i = 0; i < 16; i++)
    {
        int dig = (value >> 60) & 0xf;
        ptr[i] = dig_to_hex(dig);
        value <<= 4;
    }
}
#endif

#ifdef DEBUG_LOG_CALLS
void log_malloc_call_start(size_t size)
{
    static char buffer[64] = "malloc(0x00000000)";
    const size_t size_offset = 9;
    print_hex_32(&buffer[size_offset], size);
    write(2, buffer, strlen(buffer));
}

void log_malloc_call_end(void* ptr)
{
    static char buffer[64] = " --> 0x0000000000000000\n";
    const size_t location_offset = 7;
    print_hex_64(&buffer[location_offset], (uintptr_t)ptr);
    write(2, buffer, strlen(buffer));
}

void log_free_call(void* ptr)
{
    static char buffer[64] = "free(0x0000000000000000)\n";
    const size_t location_offset = 7;
    print_hex_64(&buffer[location_offset], (uintptr_t)ptr);
    write(2, buffer, strlen(buffer));
}
#endif

#ifdef DEBUG_LOG_HEAP_EXTENSIONS
void log_heap_extension(void* ptr)
{
    static char buffer[64] = "brk(0x0000000000000000)\n";
    const size_t location_offset = 6;
    print_hex_64(&buffer[location_offset], (uintptr_t)ptr);
    write(2, buffer, strlen(buffer));
}
#endif

#ifdef DEBUG_LOG_HEAP_TAMPERING
void log_heap_tamper(void* old_ptr, void* new_ptr)
{
    static char buffer[128] =
        "heap: tampering detected (0x0000000000000000 --> 0x0000000000000000)\n";
    const size_t old_offset = 26;
    const size_t new_offset = 49;
    print_hex_64(&buffer[old_offset], (uintptr_t)old_ptr);
    print_hex_64(&buffer[new_offset], (uintptr_t)new_ptr);
    write(2, buffer, strlen(buffer));
}
#endif