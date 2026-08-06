#include "econfig.h"
#include "emalloc.h"

// All fields are initialized to 0, no need for a constructor or an init function
static buddy_chain_head_t chain_head = {};

static inline void set_bit_in_slab_bitmap(buddy_header_t* header, size_t index, bool bit)
{
    const size_t byte_index = index / 8;
    const uint8_t bit_index = index % 8;

    if (byte_index >= BUDDY_SLAB_BITMAP_BYTES)
    {
        panic("buddy allocator: slab bitmap write fault\n");
    }

    if (bit)
    {
        (header->slab_bitmap)[byte_index] |= 1 << bit_index;
    }
    else
    {
        (header->slab_bitmap)[byte_index] &= ~(1 << bit_index);
    }
}

static inline bool get_bit_in_slab_bitmap(const buddy_header_t* header, size_t index)
{
    const size_t byte_index = index / 8;
    const uint8_t bit_index = index % 8;

    if (byte_index >= BUDDY_SLAB_BITMAP_BYTES)
    {
        panic("buddy allocator: slab bitmap read fault\n");
    }

    return ((header->slab_bitmap)[byte_index] & (1 << bit_index)) != 0;
}

static inline size_t calculate_bitmap_offset(size_t slot_size_index)
{
    size_t offset = 0;
    for (size_t i = 0; i < slot_size_index; i++)
    {
        offset |= (1 << (BUDDY_ALLOCATOR_SLOTS - i));
    }
    return offset;
}

static inline void set_bit_in_slot_bitmap(
    buddy_header_t* header, size_t slot_size_index, size_t slot_index, bool bit)
{
    const size_t offset = calculate_bitmap_offset(slot_size_index);
    const size_t index_in_bitmap = slot_index >> slot_size_index;
    const size_t byte_index = (offset + index_in_bitmap) / 8;
    const uint8_t bit_index = (offset + index_in_bitmap) % 8;

    if (byte_index >= BUDDY_SLOT_BITMAP_BYTES)
    {
        panic("buddy allocator: slot bitmap write fault\n");
    }

    if (bit)
    {
        (header->slot_bitmap)[byte_index] |= 1 << bit_index;
    }
    else
    {
        (header->slot_bitmap)[byte_index] &= ~(1 << bit_index);
    }
}

// Returns E_BIT_USED only if a given level is used
static inline bool get_bit_in_slot_bitmap(
    const buddy_header_t* header, size_t slot_size_index, size_t slot_index)
{
    const size_t offset = calculate_bitmap_offset(slot_size_index);
    const size_t index_in_bitmap = slot_index >> slot_size_index;
    const size_t byte_index = (offset + index_in_bitmap) / 8;
    const uint8_t bit_index = (offset + index_in_bitmap) % 8;

    if (byte_index >= BUDDY_SLOT_BITMAP_BYTES)
    {
        panic("buddy allocator: slot bitmap read fault\n");
    }

    return ((header->slot_bitmap)[byte_index] & (1 << bit_index)) != 0;
}

// Returns E_BIT_USED if a given level or any of it's children are used
static inline bool get_bit_and_children_in_slot_bitmap(
    const buddy_header_t* header, size_t slot_size_index, size_t slot_index)
{
    // Check all the bits in the child slots
    for (int i = slot_size_index; i >= 0; i--)
    {
        for (int j = 0; j < (1 << (slot_size_index - i)); j++)
        {
            size_t child_index = slot_index + (j << i);
            if (get_bit_in_slot_bitmap(header, i, child_index) == E_BIT_USED)
            {
                return E_BIT_USED;
            }
        }
    }

    return E_BIT_FREE;
}

// Returns E_BIT_USED if a given level, a parent or a children is used
static inline bool get_alloc_in_slot_bitmap(
    const buddy_header_t* header, size_t slot_size_index, size_t slot_index)
{
    // Check the requested slot size and all it's parent slots
    for (int i = BUDDY_ALLOCATOR_SIZES - 1; i >= (int)slot_size_index; i--)
    {
        if (get_bit_in_slot_bitmap(header, i, slot_index) == E_BIT_USED)
        {
            return E_BIT_USED;
        }
    }

    // Check all the bits in the child slots
    for (int i = slot_size_index - 1; i >= 0; i--)
    {
        for (int j = 0; j < (1 << (slot_size_index - i)); j++)
        {
            size_t child_index = slot_index + (j << i);
            if (get_bit_in_slot_bitmap(header, i, child_index) == E_BIT_USED)
            {
                return E_BIT_USED;
            }
        }
    }

    return E_BIT_FREE;
}

#ifdef DEBUG_DUMP_BUDDY_ARENAS
#include <stdio.h>
static void dump_all_arenas_in_chain(void)
{
    buddy_header_t* arena;
    int i;
    for (arena = chain_head.head_arena, i = 0; arena != NULL; arena = arena->next_arena, i++)
    {
        printf("Arena [%d]\n * Size usage: ", i);
        uint32_t total_used = 0;
        for (int j = 0; j < BUDDY_ALLOCATOR_SIZES; j++)
        {
            uint32_t slot_size_used = 0;
            for (int k = 0; k < (1 << BUDDY_ALLOCATOR_SLOTS); k += 1 << j)
            {
                if (get_bit_in_slot_bitmap(arena, j, k) == E_BIT_USED)
                {
                    slot_size_used++;
                    total_used += 1 << j;
                }
            }
            printf("%dKiB: %d", 1 << (j + BUDDY_ALLOCATOR_MIN_OBJECT - 10), slot_size_used);
            if (j < BUDDY_ALLOCATOR_SIZES - 1) printf(", ");
        }
        printf("\n * Free counters: ");
        for (int j = 0; j < BUDDY_ALLOCATOR_SIZES; j++)
        {
            printf("%dKiB: %d", 1 << (j + BUDDY_ALLOCATOR_MIN_OBJECT - 10),
                arena->free_slot_count_of_size[j]);
            if (j < BUDDY_ALLOCATOR_SIZES - 1) printf(", ");
        }
        printf("\n * Slots used: %d/%d\n", total_used, (1 << BUDDY_ALLOCATOR_SLOTS));
    }
}

static __attribute__((destructor)) void dump_buddy_arenas(void)
{
    printf("Buddy arenas allocated: %d\n", chain_head.arena_count);
    dump_all_arenas_in_chain();
}
#endif

static buddy_arena_t* allocate_new_arena(void)
{
    buddy_arena_t* arena = brk_allocate_buddy_arena();
    buddy_header_t* header = (buddy_header_t*)arena;

    header->magic1 = BUDDY_ALLOCATOR_MAGIC1;
    header->magic2 = BUDDY_ALLOCATOR_MAGIC2;

    header->index_in_chain = chain_head.arena_count++;
    header->next_arena = NULL;

    // We assume the header will only take up one slot of the arena (-1)
    for (int i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        header->free_slot_count_of_size[i] =
            (1 << (BUDDY_ALLOCATOR_ARENA_SIZE - BUDDY_ALLOCATOR_MIN_OBJECT - i)) - 1;
    }

    // Fill the bitmaps
    memset(header->slab_bitmap, 0, sizeof(header->slab_bitmap));
    memset(header->slot_bitmap, 0, sizeof(header->slot_bitmap));
    set_bit_in_slot_bitmap(header, 0, 0, E_BIT_USED);

    return arena;
}

static inline void allocate_first_arena(void)
{
    buddy_arena_t* arena = allocate_new_arena();

    chain_head.head_arena = arena;
    chain_head.tail_arena = arena;
    for (int i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        chain_head.first_free_of_size_index[i] = 0;
        chain_head.first_free_of_size[i] = arena;
    }
}

static size_t calculate_slot_size_index(size_t size)
{
    for (int i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        if (size < (1ULL << (BUDDY_ALLOCATOR_MIN_OBJECT + i)))
        {
            return i;
        }
    }

    panic("buddy allocator: alloc routing fault\n");
    return 0;
}

static uint16_t find_indices_buddy(size_t index, size_t slot_size_index)
{
    return index ^ (1 << slot_size_index);
}

static buddy_arena_t* find_arena_for_allocation(size_t slot_size_index)
{
    // If there are no arenas that can accomodate this size, create a new arena
    if (chain_head.first_free_of_size[slot_size_index] == NULL)
    {
        buddy_header_t* arena = allocate_new_arena();
        ((buddy_header_t*)chain_head.tail_arena)->next_arena = arena;
        chain_head.tail_arena = arena;

        // Update all NULL free arenas
        for (int i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
        {
            if (chain_head.first_free_of_size[i] == NULL)
            {
                chain_head.first_free_of_size[i] = arena;
                chain_head.first_free_of_size_index[i] = arena->index_in_chain;
            }
        }
    }

    return chain_head.first_free_of_size[slot_size_index];
}

static size_t find_space_in_arena(buddy_arena_t* arena, size_t slot_size_index)
{
    // Use first fit for the allocation
    const size_t area_length = 1 << slot_size_index;
    for (size_t i = 0; i < (1 << BUDDY_ALLOCATOR_SLOTS); i += area_length)
    {
        if (get_alloc_in_slot_bitmap(arena, slot_size_index, i) == E_BIT_FREE)
        {
            return i;
        }
    }

    panic("buddy allocator: corrupted arena metadata");
    return 0;
}

static void mark_space_in_arena(
    buddy_arena_t* arena, size_t slot_size_index, size_t slot_index, bool state)
{
    set_bit_in_slot_bitmap(arena, slot_size_index, slot_index, state);
}

static buddy_arena_t* find_next_arena_with_free_slot(buddy_arena_t* arena, size_t slot_size_index)
{
    while (((buddy_header_t*)arena)->next_arena != NULL)
    {
        buddy_arena_t* next_arena = ((buddy_header_t*)arena)->next_arena;
        buddy_header_t* next_header = next_arena;

        if (next_header->free_slot_count_of_size[slot_size_index] > 0)
        {
            return next_arena;
        }
        else
        {
            arena = next_arena;
        }
    }

    return NULL;
}

static void update_chain_head_first_free(buddy_arena_t* arena, size_t slot_size_index)
{
    buddy_arena_t* next_free = find_next_arena_with_free_slot(arena, slot_size_index);
    chain_head.first_free_of_size[slot_size_index] = next_free;
    if (next_free != NULL)
    {
        uint32_t next_free_index = ((buddy_header_t*)next_free)->index_in_chain;
        chain_head.first_free_of_size_index[slot_size_index] = next_free_index;
    }
}

static size_t allocate_in_arena(buddy_arena_t* arena, size_t slot_size_index)
{
    buddy_header_t* header = (buddy_header_t*)arena;

    // Find the space and mark it as used
    size_t slot_index = find_space_in_arena(arena, slot_size_index);
    mark_space_in_arena(arena, slot_size_index, slot_index, E_BIT_USED);

    // Update the size counts of object smaller or the same in size
    for (size_t i = 0; i <= slot_size_index; i++)
    {
        header->free_slot_count_of_size[i] -= 1 << (slot_size_index - i);

        // Update the pointers in head
        if (header->free_slot_count_of_size[i] == 0)
        {
            update_chain_head_first_free(arena, i);
        }
    }

    // Update the larger slots if their buddies are not allocated
    for (size_t i = slot_size_index + 1; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        size_t buddy = find_indices_buddy(slot_index, i - 1);

        // If the buddy is allocated, we shouldn't decrease this or any larger slot counter
        if (get_bit_and_children_in_slot_bitmap(arena, i - 1, buddy) == E_BIT_USED)
        {
            break;
        }

        header->free_slot_count_of_size[i]--;

        // Update the pointers in head
        if (header->free_slot_count_of_size[i] == 0)
        {
            update_chain_head_first_free(arena, i);
        }
    }

    return slot_index;
}

static void* slot_index_to_ptr(buddy_arena_t* arena, size_t slot_index)
{
    // Resolve the allocated address and return it
    return (void*)((uintptr_t)arena + (slot_index << BUDDY_ALLOCATOR_MIN_OBJECT));
}

static size_t slot_index_to_slab_slot_index(size_t slot_index)
{
    return slot_index >> (SLAB_ALLOCATOR_SLAB_SIZE - BUDDY_ALLOCATOR_MIN_OBJECT);
}

static void mark_slot_as_slab(buddy_arena_t* arena, size_t slot_index)
{
    const size_t slab_slot_index = slot_index_to_slab_slot_index(slot_index);
    set_bit_in_slab_bitmap(arena, slab_slot_index, E_BIT_USED);
}

bool is_buddy_header(buddy_header_t* arena)
{
    return (arena->magic1 == BUDDY_ALLOCATOR_MAGIC1 && arena->magic2 == BUDDY_ALLOCATOR_MAGIC2);
}

static bool is_slab_index(buddy_arena_t* arena, size_t slot_index)
{
    const size_t slab_slot_index = slot_index_to_slab_slot_index(slot_index);
    return get_bit_in_slab_bitmap(arena, slab_slot_index) == E_BIT_USED;
}

int find_index_in_arena(void* arena, void* ptr, size_t* slot_size_index)
{
    if (!is_buddy_header(arena))
    {
        return -E_RESULT_ERROR;
    }

    const int slot_index = ((uintptr_t)(ptr - arena)) >> BUDDY_ALLOCATOR_MIN_OBJECT;

    // Return early if slab slot hit
    if (is_slab_index(arena, slot_index))
    {
        return -E_RESULT_SLAB_ALLOCATION;
    }

    for (size_t i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        // Skip if allocation isn't aligned
        if (((uintptr_t)ptr & ((1 << (i + BUDDY_ALLOCATOR_MIN_OBJECT)) - 1)) != 0) continue;

        // Skip if the allocation isn't marked in the bitmap
        if (get_bit_in_slot_bitmap(arena, i, slot_index) == E_BIT_FREE) continue;

        // Return the index and slot size
        *slot_size_index = i;
        return slot_index;
    }

    return -E_RESULT_ERROR;
}

static void free_and_update_counters(
    buddy_arena_t* arena, size_t slot_index, size_t slot_size_index)
{
    buddy_header_t* header = arena;

    // Update the bitmap
    set_bit_in_slot_bitmap(arena, slot_size_index, slot_index, E_BIT_FREE);

    // Update the counter and the child counters
    for (size_t i = 0; i <= slot_size_index; i++)
    {
        size_t increment = 1 << (slot_size_index - i);
        header->free_slot_count_of_size[i] += increment;

        // Update the chain head free pointers
        if (header->free_slot_count_of_size[i] == increment)
        {
            if (header->index_in_chain < chain_head.first_free_of_size_index[i])
            {
                chain_head.first_free_of_size[i] = arena;
                chain_head.first_free_of_size_index[i] = header->index_in_chain;
            }
        }
    }

    // Update the parent counters
    for (int i = slot_size_index; i < BUDDY_ALLOCATOR_SIZES - 1; i++)
    {
        // Don't update higher counters if the buddy is still allocated
        const size_t aligned_slot_index = slot_index & ~((1 << i) - 1);
        const size_t buddy = find_indices_buddy(aligned_slot_index, i);
        if (get_bit_and_children_in_slot_bitmap(arena, i, buddy) == E_BIT_USED)
        {
            break;
        }

        header->free_slot_count_of_size[i + 1]++;

        // Update the chain head free pointers
        if (header->free_slot_count_of_size[i + 1] == 1)
        {
            if (header->index_in_chain < chain_head.first_free_of_size_index[i + 1])
            {
                chain_head.first_free_of_size[i + 1] = arena;
                chain_head.first_free_of_size_index[i + 1] = header->index_in_chain;
            }
        }
    }
}

#ifdef DEBUG_VERIFY_BUDDY_FREE_COUNTERS
static inline void verify_arena_free_counters(buddy_header_t* header)
{
    for (int i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        const size_t max_free = 1 << (BUDDY_ALLOCATOR_SLOTS - i);
        if (header->free_slot_count_of_size[i] >= max_free)
        {
            panic("buddy allocator: free counters corrupted\n");
        }
    }
}
#endif

#ifdef DEBUG_VERIFY_BUDDY_BITMAPS
static inline void verify_arena_bitmaps(buddy_header_t* header)
{
    for (int i = 0; i < BUDDY_ALLOCATOR_SIZES; i++)
    {
        uint32_t free_slots = 0;
        for (int j = 0; j < (1 << BUDDY_ALLOCATOR_SLOTS); j += (1 << i))
        {
            if (get_alloc_in_slot_bitmap(header, i, j) == E_BIT_FREE)
            {
                free_slots++;
            }
        }

        if (free_slots != header->free_slot_count_of_size[i])
        {
            panic("buddy allocator: bitmaps corrupted\n");
        }
    }
}
#endif

static void* buddy_alloc_internal(size_t size, bool mark_slab)
{
    // Allocate the first block if needed
    if (chain_head.head_arena == NULL)
    {
        allocate_first_arena();
    }

    size_t slot_size_index = calculate_slot_size_index(size);
    buddy_arena_t* arena = find_arena_for_allocation(slot_size_index);
    size_t slot_index = allocate_in_arena(arena, slot_size_index);
    void* ptr = slot_index_to_ptr(arena, slot_index);

    if (mark_slab)
    {
        mark_slot_as_slab(arena, slot_index);
    }

#ifdef DEBUG_VERIFY_BUDDY_FREE_COUNTERS
    verify_arena_free_counters(arena);
#endif

#ifdef DEBUG_VERIFY_BUDDY_BITMAPS
    verify_arena_bitmaps(arena);
#endif

    return ptr;
}

void* buddy_alloc(size_t size)
{
    return buddy_alloc_internal(size, false);
}

void* buddy_alloc_slab(size_t size)
{
    return buddy_alloc_internal(size, true);
}

void buddy_free(void* ptr)
{
    const uintptr_t arena_mask = ~((1ULL << BUDDY_ALLOCATOR_ARENA_SIZE) - 1);
    buddy_arena_t* arena = (buddy_arena_t*)((uintptr_t)ptr & arena_mask);
    buddy_header_t* header = (buddy_arena_t*)arena;

    size_t slot_size_index;
    int slot_index = find_index_in_arena(arena, ptr, &slot_size_index);

    // If it's a slab slot, route free to the slab deallocator
    if (slot_index == -E_RESULT_SLAB_ALLOCATION)
    {
        slab_free(ptr);
        return;
    }

    // If failed to find the index, don't deallocate
    if (slot_index < 0)
    {
#ifdef DEBUG_PANIC_ON_FREE_FAIL
        panic("buddy allocator: free failed\n");
#endif
        return;
    }

    free_and_update_counters(header, slot_index, slot_size_index);

#ifdef DEBUG_VERIFY_BUDDY_FREE_COUNTERS
    verify_arena_free_counters(arena);
#endif

#ifdef DEBUG_VERIFY_BUDDY_BITMAPS
    verify_arena_bitmaps(arena);
#endif
}

size_t buddy_get_realloc_size(void* ptr)
{
    const uintptr_t arena_mask = ~((1ULL << BUDDY_ALLOCATOR_ARENA_SIZE) - 1);
    buddy_arena_t* arena = (buddy_arena_t*)((uintptr_t)ptr & arena_mask);

    size_t slot_size_index;
    int slot_index = find_index_in_arena(arena, ptr, &slot_size_index);

    // If it's a slab slot, route free to the slab deallocator
    if (slot_index == -E_RESULT_SLAB_ALLOCATION)
    {
        return slab_get_realloc_size(ptr);
    }

    if (slot_index < 0)
    {
        panic("buddy allocator: realloc failed\n");
        return 0;
    }

    return 1 << (slot_size_index + BUDDY_ALLOCATOR_MIN_OBJECT);
}