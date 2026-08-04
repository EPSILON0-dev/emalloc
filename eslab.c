#include "emalloc.h"

static slab_chain_head_t slab_heads[SLAB_ALLOCATOR_SLAB_COUNT];

static __attribute__((constructor)) void init_slab_heads(void)
{
    for (int i = 0; i < SLAB_ALLOCATOR_SLAB_COUNT; i++)
    {
        slab_heads[i].first_free_index = 0;
        slab_heads[i].slab_count = 0;
        slab_heads[i].object_size = 1 << (SLAB_ALLOCATOR_MIN_OBJECT + i);
        slab_heads[i].first_free_slab = NULL;
        slab_heads[i].first_slab = NULL;
    }
}

#ifdef DEBUG_DUMP_SLAB_CHAINS
#include <stdio.h>
static uint8_t dump_bit_reverse(uint8_t value)
{
    uint8_t out = 0;
    uint8_t inmask = 0x01, outmask = 0x80;
    for (int i = 0; i < 8; i++)
    {
        if ((value & inmask) != 0) out |= outmask;
        inmask <<= 1;
        outmask >>= 1;
    }
    return out;
}

static void dump_all_slabs_in_chain(slab_chain_head_t* head)
{
    int j;
    slab_header_t* slab;
    for (slab = head->first_slab, j = 0; slab != NULL; slab = slab->next_slab, j++)
    {
        uint32_t total = slab->object_count - slab->first_usable_index;
        uint32_t used = total - slab->free_object_count;

        if (used == 0) continue;

        printf(" * [%u]: %u/%u used, bitmap: ", j, used, total);
        for (int k = 0; k < slab->object_count / 8 && k < 8; k++)
        {
            printf("%02x", dump_bit_reverse(slab->bitmap[k]));
            if (k == 7 && slab->object_count / 8 > 8) printf("... (* trunc *)");
        }
        printf("\n");
    }
    printf(" * (* truncated empty slabs *)\n");
}

static __attribute__((destructor)) void dump_slab_chains(void)
{
    for (int i = 0; i < SLAB_ALLOCATOR_SLAB_COUNT; i++)
    {
        if (slab_heads[i].slab_count == 0) continue;

        const uint32_t obj_size = slab_heads[i].object_size;
        const uint32_t slab_count = slab_heads[i].slab_count;
        printf("Slab chain %d, allocated: %d\n", obj_size, slab_count);
        dump_all_slabs_in_chain(&slab_heads[i]);
    }
}
#endif

static inline slab_chain_head_t* get_slab_chain_for_allocation(size_t size)
{
    for (int i = 0; i < SLAB_ALLOCATOR_SLAB_COUNT; i++)
    {
        if (size <= slab_heads[i].object_size) return &slab_heads[i];
    }

    panic("slab allocator: alloc routing fault\n");
    return 0;
}

static inline void set_bit_in_bitmap(slab_header_t* header, size_t index, bool bit)
{
    const size_t byte_index = index / 8;
    const uint8_t bit_index = index % 8;
    const size_t bitmap_bytes = header->object_count / 8;

    if (byte_index >= bitmap_bytes)
    {
        panic("slab allocator: bitmap write fault\n");
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
        panic("slab allocator: bitmap read fault\n");
    }

    return ((header->bitmap)[byte_index] & (1 << bit_index)) != 0;
}

static slab_t* allocate_new_slab(slab_chain_head_t* head)
{
    const size_t slab_size = (1 << SLAB_ALLOCATOR_SLAB_SIZE);
    const size_t object_size = head->object_size;

    slab_t* slab = buddy_alloc_slab(slab_size);
    slab_header_t* header = (slab_header_t*)slab;

    size_t bitmap_bytes = slab_size / object_size / 8 + !!(slab_size / object_size % 8);
    size_t header_size = sizeof(slab_header_t) + bitmap_bytes;
    size_t objects_used_by_header = header_size / object_size;
    objects_used_by_header += (header_size % object_size != 0 ? 1 : 0);

    header->magic1 = SLAB_ALLOCATOR_MAGIC;
    header->magic2 = SLAB_ALLOCATOR_MAGIC;

    header->object_size = object_size;

    header->object_count = slab_size / object_size;
    header->free_object_count = header->object_count - objects_used_by_header;

    header->first_usable_index = objects_used_by_header;
    header->first_free_index = header->first_usable_index;

    header->head = head;
    header->index_in_chain = head->slab_count++;
    header->next_slab = NULL;

    memset(header->bitmap, 0, bitmap_bytes);
    for (size_t i = 0; i < objects_used_by_header; i++)
    {
        set_bit_in_bitmap(header, i, E_BIT_USED);
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
    set_bit_in_bitmap(header, allocated_index, E_BIT_USED);
    header->free_object_count--;

    // We have the bitmap in cache, we might as well find the next free spot
    if (header->free_object_count > 0)
    {
        bool found_free_index = false;
        for (int i = allocated_index; i < header->object_count; i++)
        {
            if (get_bit_in_bitmap(header, i) == E_BIT_FREE)
            {
                header->first_free_index = i;
                found_free_index = true;
                break;
            }
        }

        if (!found_free_index)
        {
            panic("slab allocator: corrupted slab bitmap\n");
        }
    }

    return allocated_index;
}

static void* allocate_in_slab_chain(slab_chain_head_t* head)
{
    // Allocate the first slab if it doesn't exist
    if (head->first_slab == NULL)
    {
        head->first_slab = allocate_new_slab(head);
        head->first_free_slab = head->first_slab;
        head->first_free_index = 0;
    }

    // Allocate memory in the currently non-full slab
    slab_t* slab = head->first_free_slab;
    const slab_t* allocated_slab = slab;
    const slab_header_t* allocated_slab_header = (slab_header_t*)slab;
    const int allocated_index = allocate_index_in_slab(slab);

    // If the slab is non-empty the metadata lied, panic
    if (allocated_index < 0)
    {
        panic("slab allocator: corrupted slab chain head\n");
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
                head->first_free_index = header->index_in_chain;
                break;
            }

            // If all slabs are used, allocate an new one
            if (header->next_slab == NULL)
            {
                header->next_slab = allocate_new_slab(head);
            }

            slab = header->next_slab;
        }
    }

    // Resolve the address of the allocation and return it
    const size_t offset_in_slab = (allocated_slab_header->object_size * (size_t)allocated_index);
    return (void*)((uintptr_t)allocated_slab + offset_in_slab);
}

bool is_slab_header(slab_header_t* slab)
{
    return (slab->magic1 == SLAB_ALLOCATOR_MAGIC && slab->magic2 == SLAB_ALLOCATOR_MAGIC);
}

int find_index_in_slab(slab_t* slab, void* ptr)
{
    slab_header_t* header = (slab_header_t*)slab;

    // Return if the object is misaligned
    if (((uintptr_t)ptr & (header->object_size - 1)) != 0)
    {
        return E_RESULT_ERROR;
    }

    // Return if the we failed to locate the slab header
    if (!is_slab_header(header))
    {
        return E_RESULT_ERROR;
    }

    // Return the index in the slab
    uintptr_t offset_in_slab = (uintptr_t)(ptr - slab);
    return offset_in_slab / header->object_size;
}

void* slab_alloc(size_t size)
{
    if (size > (1 << SLAB_ALLOCATOR_MAX_OBJECT))
    {
        panic("slab allocator: alloc routing fault\n");
    }

    slab_chain_head_t* slab_chain = get_slab_chain_for_allocation(size);
    return allocate_in_slab_chain(slab_chain);
}

void slab_free(void* ptr)
{
    slab_t* slab = (slab_t*)((uintptr_t)ptr & ~((1ULL << SLAB_ALLOCATOR_SLAB_SIZE) - 1));
    slab_header_t* header = (slab_header_t*)slab;
    int index = find_index_in_slab(slab, ptr);

    // If finding the index failed, don't free
    if (index < 0)
    {
#ifdef DEBUG_PANIC_ON_FREE_FAIL
        panic("slab allocator: free failed\n");
#endif
        return;
    }

    // Clear the bit in the bitmap and update the free count
    set_bit_in_bitmap(header, index, E_BIT_FREE);
    header->free_object_count++;

    // Update the slab's first free
    if (header->free_object_count == 1 || index < header->first_free_index)
    {
        header->first_free_index = index;
    }

    // Update the chain's first free
    if (header->free_object_count == 1 && header->index_in_chain < header->head->first_free_index)
    {
        header->head->first_free_index = header->index_in_chain;
        header->head->first_free_slab = slab;
    }
}