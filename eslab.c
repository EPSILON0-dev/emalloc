#include "econfig.h"
#include "emalloc.h"

static slab_chain_head_t slab_heads[SLAB_ALLOCATOR_SLAB_COUNT];

static __attribute__((constructor)) void init_slab_heads(void)
{
    const static uint8_t slab_sizes[] = SLAB_ALLOCATOR_SLAB_SIZES;

    for (int i = 0; i < SLAB_ALLOCATOR_SLAB_COUNT; i++)
    {
        slab_heads[i].slab_size = slab_sizes[i];
        slab_heads[i].object_size = 1 << (SLAB_ALLOCATOR_MIN_OBJECT + i);
        slab_heads[i].first_free_slab = NULL;
        slab_heads[i].first_slab = NULL;
    }
}

static inline slab_chain_head_t* get_slab_chain_for_allocation(size_t size)
{
    for (int i = 0; i < SLAB_ALLOCATOR_SLAB_COUNT; i++)
    {
        if (size <= slab_heads[i].object_size) return &slab_heads[i];
    }

    panic("slab allocator: alloc routing fault");
    return 0;
}

static inline void set_bit_in_bitmap(slab_header_t* header, size_t index, bool bit)
{
    const size_t byte_index = index / 8;
    const uint8_t bit_index = index % 8;
    const size_t bitmap_bytes = header->object_count / 8;

    if (byte_index >= bitmap_bytes)
    {
        panic("slab allocator: bitmap write fault");
    }

    if (bit)
    {
        (header->bitmap)[byte_index] |= 1 << bit_index;
    }
    else
    {
        (header->bitmap)[byte_index] &= ~(1 << bit_index);
    }
}

static inline bool get_bit_in_bitmap(const slab_header_t* header, size_t index)
{
    const size_t byte_index = index / 8;
    const uint8_t bit_index = index % 8;
    const size_t bitmap_bytes = header->object_count / 8;

    if (byte_index >= bitmap_bytes)
    {
        panic("slab allocator: bitmap read fault");
    }

    return ((header->bitmap)[byte_index] & (1 << bit_index)) != 0;
}

static slab_t* allocate_slab_in_heap(size_t object_size, size_t slab_size)
{
    slab_t* slab = brk_alloc(slab_size);
    slab_header_t* header = (slab_header_t*)slab;

    size_t bitmap_bytes = header->object_count / 8;
    size_t header_size = sizeof(slab_header_t) + bitmap_bytes;

    size_t objects_used_by_header = header_size / object_size;
    objects_used_by_header += (header_size % object_size != 0 ? 1 : 0);

    header->slab_size = slab_size;
    header->object_size = object_size;

    header->object_count = (slab_size - sizeof(slab_header_t)) / object_size;
    header->free_object_count = header->object_count - objects_used_by_header;

    header->first_usable_index = objects_used_by_header;
    header->first_free_index = header->first_usable_index;

    header->next_slab = NULL;

    memset(header->bitmap, 0, bitmap_bytes);
    for (int i = 0; i < objects_used_by_header; i++)
    {
        set_bit_in_bitmap(header, i, 1);
    }

    return slab;
}

static int allocate_index_in_slab(slab_t* slab)
{
    slab_header_t* header = (slab_header_t*)slab;

    // Check if there's space in the slab to allocate
    if (header->free_object_count == 0)
    {
        return -ENOMEM;
    }

    // Allocate the index and mark it as used
    size_t allocated_index = header->first_free_index;
    set_bit_in_bitmap(header, allocated_index, 1);
    header->free_object_count--;

    // We have the bitmap in cache, we might as well find the next free spot
    if (header->free_object_count > 0)
    {
        bool found_free_index = false;
        for (int i = allocated_index; i < header->object_count; i++)
        {
            if (get_bit_in_bitmap(header, i) == 0)
            {
                header->first_free_index = i;
                found_free_index = true;
                break;
            }
        }

        if (!found_free_index)
        {
            panic("slab allocator: corrupted slab bitmap");
        }
    }

    return allocated_index;
}

static void* allocate_in_slab_chain(slab_chain_head_t* head)
{
    // Allocate the first slab if it doesn't exist
    if (head->first_slab == NULL)
    {
        head->first_slab = allocate_slab_in_heap(head->object_size, head->slab_size);
        head->first_free_slab = head->first_free_slab;
    }

    // Allocate memory in the currently non-full slab
    slab_t* slab = head->first_free_slab;
    const slab_t* allocated_slab = slab;
    const slab_header_t* allocated_slab_header = (slab_header_t*)slab;
    const int allocated_index = allocate_index_in_slab(slab);

    // If the slab is non-empty the metadata lied, panic
    if (allocated_index < 0)
    {
        panic("slab allocator: corrupted slab chain head");
    }

    // Find the next slab with free slots
    if (((slab_header_t*)slab)->free_object_count == 0)
    {
        while (slab != NULL)
        {
            slab_header_t* header = ((slab_header_t*)slab);

            if (header->free_object_count > 0)
            {
                head->first_free_slab = slab;
                break;
            }

            // If all slabs are used, allocate an new one
            if (header->next_slab == NULL)
            {
                header->next_slab = allocate_slab_in_heap(head->object_size, head->slab_size);
            }

            slab = header->next_slab;
        }
    }

    // Resolve the address of the allocation and return it
    const size_t offset_in_slab = (allocated_slab_header->object_size * (size_t)index);
    return (void*)((uintptr_t)allocated_slab + offset_in_slab);
}

void* slab_alloc(size_t size)
{
    if (size > (1 << SLAB_ALLOCATOR_MAX_OBJECT))
    {
        panic("slab allocator: alloc routing fault");
    }

    slab_chain_head_t* slab_chain = get_slab_chain_for_allocation(size);
    return allocate_in_slab_chain(slab_chain);
}