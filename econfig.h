#ifndef ECONFIG_H
#define ECONFIG_H

// It's called a config, but changing the allocation sizes will most likely break the whole thing :p

// Print out the slab chains when the program exits
// #define DEBUG_DUMP_SLAB_CHAINS

// Check if buddy arena free counters were corrupted after every alloc or free
// #define DEBUG_VERIFY_BUDDY_FREE_COUNTERS

// Check if bitmaps match free counters after every alloc or free
// #define DEBUG_VERIFY_BUDDY_BITMAPS

// Print out the buddy arenas when the program exits
// #define DEBUG_DUMP_BUDDY_ARENAS

// Panic when a free call fails instead of skipping deallocation
// #define DEBUG_PANIC_ON_FREE_FAIL

// Log all calls without using the allocator for printf
// #define DEBUG_LOG_CALLS

// Log heap extensions with brk 
// #define DEBUG_LOG_HEAP_EXTENSIONS

// Log a stuck spinlock if stuck for too long
// #define DEBUG_LOG_STUCK_SPINLOCKS

// All values below are powers of 2 (10 would be 2^10 = 1024, 11 would be 2 ^ 11 = 2048)

// Increments of brk calls, usually the bigger the better as the kernel lazily allocates
//  the brk space
#define BRK_ENLARGE_INCREMENT 23  // 8MiB

// The allocation size with which mmap is called directly
#define DIRECT_MMAP_THRESHOLD 20  // 1MiB

// Magic values to be used in the slab allocator's headers
#define SLAB_ALLOCATOR_MAGIC 0x11ff22ff

// The maximum and minimum object size for slab allocator
#define SLAB_ALLOCATOR_MAX_OBJECT 13  // 8KiB
#define SLAB_ALLOCATOR_MIN_OBJECT 5   // 32B
#define SLAB_ALLOCATOR_SLAB_COUNT (SLAB_ALLOCATOR_MAX_OBJECT - SLAB_ALLOCATOR_MIN_OBJECT + 1)

// Slab sizes for each object size
#define SLAB_ALLOCATOR_SLAB_SIZE 16  // 64KiB

// Magic values to be used in the buddy allocator's headers
#define BUDDY_ALLOCATOR_MAGIC1 0xff88ff99
#define BUDDY_ALLOCATOR_MAGIC2 0x3c33

// The maximum and minimum object size for slab allocator
#define BUDDY_ALLOCATOR_MAX_OBJECT 20  // 1MiB
#define BUDDY_ALLOCATOR_MIN_OBJECT 14  // 16KiB

// The size of a single chunk of memory a buddy allocator can allocate
#define BUDDY_ALLOCATOR_ARENA_SIZE 23  // 8MiB

#define BUDDY_ALLOCATOR_SLOTS (BUDDY_ALLOCATOR_ARENA_SIZE - BUDDY_ALLOCATOR_MIN_OBJECT)
#define BUDDY_ALLOCATOR_SLOTS_PER_SLAB (SLAB_ALLOCATOR_SLAB_SIZE - BUDDY_ALLOCATOR_MIN_OBJECT)
#define BUDDY_ALLOCATOR_SIZES (BUDDY_ALLOCATOR_MAX_OBJECT - BUDDY_ALLOCATOR_MIN_OBJECT + 1)
#define BUDDY_ALLOCATOR_SLAB_SLOTS (BUDDY_ALLOCATOR_SLOTS - BUDDY_ALLOCATOR_SLOTS_PER_SLAB)

#define BUDDY_SLAB_BITMAP_BYTES ((1 << BUDDY_ALLOCATOR_SLAB_SLOTS) / 8)
#define BUDDY_SLOT_BITMAP_BYTES ((1 << BUDDY_ALLOCATOR_SLOTS) / 8 * 2)

// Magic values to be used in mmaped allocation headers
#define MMAP_ALLOCATION_MAGIC 0xf12fc343

// Page size for mmap allocations
#define MMAP_PAGE_SIZE 12

// Sanity checks
#if BUDDY_ALLOCATOR_MIN_OBJECT - SLAB_ALLOCATOR_MAX_OBJECT != 1
#error "Hybrid allocator gap, allocations can be too small for buddy and too large for slab"
#endif

#if DIRECT_MMAP_THRESHOLD > BUDDY_ALLOCATOR_MAX_OBJECT
#error "Direct mmap too large for the buddy allocator to handle"
#endif

#if BRK_ENLARGE_INCREMENT - BUDDY_ALLOCATOR_ARENA_SIZE < 0
#warning "brk increment too small, multiple syscalls per buddy arena allocation"
#endif

#if SLAB_ALLOCATOR_SLAB_SIZE - SLAB_ALLOCATOR_MAX_OBJECT < 2
#warning "Slab size too small, high memory waste with large objects"
#endif

#if BUDDY_ALLOCATOR_ARENA_SIZE - SLAB_ALLOCATOR_SLAB_SIZE > 7
#warning "Slab size too small, slab map too large for buddy allocator headers"
#endif

#if BUDDY_ALLOCATOR_SLOTS > 9
#warning "Too many buddy allocator slots"
#endif

#if MMAP_PAGE_SIZE != 12
#warning "Uncommon page size, if the page size is really not 4KiB, ignore this warning"
#endif


#endif