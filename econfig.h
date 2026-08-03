#ifndef ECONFIG_H
#define ECONFIG_H

// All values below are powers of 2 (10 would be 2^10 = 1024, 11 would be 2 ^ 11 = 2048)

// The allocation size with which mmap is called directly
#define DIRECT_MMAP_THRESHOLD 20  // 1MiB

// Increments of brk calls, usually the bigger the better as the kernel lazily allocates
//  the brk space
#define BRK_ENLARGE_INCREMENT 23 // 8MiB

// The maximum and minimum object size for slab allocator
#define SLAB_ALLOCATOR_MAX_OBJECT 13  // 8KiB
#define SLAB_ALLOCATOR_MIN_OBJECT 5   // 32B
#define SLAB_ALLOCATOR_SLAB_COUNT (SLAB_ALLOCATOR_MAX_OBJECT - SLAB_ALLOCATOR_MIN_OBJECT + 1)

// Slab sizes for each object size
#define SLAB_ALLOCATOR_SLAB_SIZES \
    {                             \
        16, /* 32B   - 64KiB  */  \
        16, /* 64B   - 64KiB  */  \
        16, /* 128B  - 64KiB  */  \
        16, /* 256B  - 64KiB  */  \
        17, /* 512B  - 128KiB */  \
        17, /* 1KiB  - 128KiB */  \
        18, /* 2KiB  - 256KiB */  \
        18, /* 4KiB  - 256KiB */  \
        19, /* 8KiB  - 512KiB */  \
    }

#endif