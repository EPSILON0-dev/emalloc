#include <sys/mman.h>

#include "econfig.h"
#include "emalloc.h"

static bool verify_mmap_header(void* ptr)
{
    mmap_header_t* header = (void*)((uintptr_t)ptr - (1 << MMAP_PAGE_SIZE));

    // Fail on alignment mismatch
    if (((uintptr_t)ptr & ((1 << MMAP_PAGE_SIZE) - 1)) != 0)
    {
        return false;
    }

    // Fail on magic mismatch
    if (header->magic1 != MMAP_ALLOCATION_MAGIC) return false;
    if (header->magic2 != MMAP_ALLOCATION_MAGIC) return false;

    // Fail on address mismatch
    if ((uintptr_t)header->allocation_address != (uintptr_t)ptr)
    {
        return false;
    }

    return true;
}

void* mmap_alloc(size_t size)
{
    size_t allocation_pages = size >> MMAP_PAGE_SIZE;
    allocation_pages += ((size & ((1 << MMAP_PAGE_SIZE) - 1)) != 0) ? 1 : 0;
    allocation_pages += 1;  // Page for the header

    size_t allocation_length = allocation_pages << MMAP_PAGE_SIZE;
    void* raw_ptr = mmap(NULL,        // address
        allocation_length,            // length
        PROT_READ | PROT_WRITE,       // protection
        MAP_PRIVATE | MAP_ANONYMOUS,  // flags
        -1,                           // file descriptor
        0                             // offset
    );

    if (raw_ptr == NULL)
    {
        panic("mmap allocator: mmap failed\n");
    }

    mmap_header_t* header = raw_ptr;
    void* ptr = (void*)((uintptr_t)raw_ptr + (1 << MMAP_PAGE_SIZE));

    header->magic1 = MMAP_ALLOCATION_MAGIC;
    header->magic2 = MMAP_ALLOCATION_MAGIC;
    header->allocation_address = ptr;
    header->allocation_length = allocation_length;

    return ptr;
}

void mmap_free(void* ptr)
{
    if (!verify_mmap_header(ptr))
    {
#ifdef DEBUG_PANIC_ON_FREE_FAIL
        panic("mmap allocator: free failed\n");
#endif
        return;
    }

    mmap_header_t* header = (mmap_header_t*)((uintptr_t)ptr - (1 << MMAP_PAGE_SIZE));
    if (munmap(header, header->allocation_length) != 0)
    {
        panic("mmap allocator: munmap failed\n");
    }
}

size_t mmap_get_realloc_size(void* ptr)
{
    if (!verify_mmap_header(ptr))
    {
        panic("mmap allocator: realloc failed\n");
        return 0;
    }

    mmap_header_t* header = (mmap_header_t*)((uintptr_t)ptr - (1 << MMAP_PAGE_SIZE));
    return header->allocation_length - (1 << MMAP_PAGE_SIZE);
}