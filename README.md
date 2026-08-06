# emalloc — A Custom Memory Allocator

emalloc is a simple, hybrid memory allocator implementation in C featuring three distinct allocation strategies optimized for different allocation sizes. It provides drop-in replacements for standard `malloc`, `free`, `calloc`, and `realloc` with comprehensive debugging capabilities.

## Architecture

The allocator uses a three-tier strategy to optimize for different allocation patterns:

### 1. Slab Allocator
Handles small allocations in the range of **32 bytes to 8 KiB**.

- Fixed-size object allocation within slabs (64 KiB per slab)
- Efficient bitmap-based free tracking
- Minimal fragmentation for small objects
- Separate chains for each object size class

### 2. Buddy Allocator
Handles medium allocations in the range of **16 KiB to 1 MiB**.

- Power-of-2 based allocation strategy
- Efficient memory coalescing on free
- Manages large arenas (8 MiB each) to reduce fragmentation
- Integrates slab allocations within buddy-managed arenas

### 3. mmap Allocator
Handles large allocations **>1 MiB**.

- Direct memory mapping via `mmap()`
- Minimal overhead for single large allocations
- Proper alignment and deallocation tracking

## Features

- **Drop-in replacement**: Compatible with `malloc()`, `free()`, `calloc()`, and `realloc()`
- **Thread-safe**: Global spinlock protects all allocator operations

## Building

### Requirements
- GCC or compatible C compiler
- Linux environment (uses `brk()` and `mmap()` system calls)
- GNU Make

### Build Commands

```bash
# Build optimized release version
make build

# Build with debug symbols
make VARIANT=DEBUG build

# Clean build artifacts
make clean

# Clean and rebuild
make fresh
```

The build produces `libemalloc.so`, a shared library that can be preloaded via `LD_PRELOAD`.

## Usage

### As a Preloaded Library

```bash
LD_PRELOAD=./libemalloc.so ./your_program
```

### Direct Linking

Include the header and link against the library:

```c
#include "emalloc.h"

int main() {
    void* ptr = emalloc(1024);
    // Use ptr...
    efree(ptr);
    return 0;
}
```

## Testing

A test harness (`test/run_trace.c`) validates allocator correctness by replaying allocation traces:

```bash
./test/run_trace <path/to/libemalloc.so> <trace_file>
```

The trace format is:
- Header: `<total_ops> <num_allocs> <num_frees>`
- Operations: `a <id> <size>` (allocate) or `f <id>` (free)

Example trace:
```
3 2 1
a 0 128
a 1 256
f 0
```

The test verifies:
- Allocation success and correct size tracking
- Data integrity across allocate-free cycles

## API Reference

### Core Functions

```c
void* emalloc(size_t size);           // Allocate memory
void* ecalloc(size_t count, size_t size); // Allocate and zero
void* erealloc(void *ptr, size_t size);   // Resize allocation
void efree(void* ptr);                 // Deallocate memory
```

### Internal Functions

- **Slab allocator**: `slab_alloc()`, `slab_free()`, `slab_get_realloc_size()`
- **Buddy allocator**: `buddy_alloc()`, `buddy_free()`, `buddy_get_realloc_size()`, `buddy_alloc_slab()`
- **mmap allocator**: `mmap_alloc()`, `mmap_free()`, `mmap_get_realloc_size()`
- **Heap management**: `brk_allocate_buddy_arena()`, `brk_get_allocated_heap_end()`
- **Synchronization**: `spin_lock()`, `spin_unlock()`

## Implementation Details

### Memory Tracking

All allocations are tracked via headers:
- **Slab headers** contain magic numbers, object size/count, free lists, and bitmap
- **Buddy headers** contain magic numbers, arena bookkeeping, and per-size bitmaps
- **mmap headers** contain magic numbers and allocation metadata

### Hybrid Strategy

The allocator seamlessly transitions between strategies:
1. Small allocations ≤8 KiB → slab
2. Medium allocations 8 KiB-1 MiB → buddy
3. Large allocations >1 MiB → mmap

The buddy allocator can internally use slab allocations for efficient medium-object handling.

### Thread Safety

A global spinlock serializes all allocator operations, ensuring thread-safe access to shared data structures. The lock is held for minimal duration during fast-path allocations.

## Limitations and Notes

- All configuration values are powers of 2 for efficient bit manipulation
- Changing allocation size parameters may break existing functionality (see `econfig.h` for constraints)
- The allocator assumes 4 KiB (2^12) page size for mmap operations
- Spinlock implementation may not be optimal for highly contended scenarios

## File Structure

- **emalloc.h/c**: Main API and dispatch logic
- **eslab.c**: Slab allocator implementation
- **ebuddy.c**: Buddy allocator implementation
- **emmap.c**: mmap-based allocator
- **ebrk.c**: Heap management via `brk()`
- **espinlock.c**: Spinlock synchronization
- **eutil.c**: Utility functions
- **eapi.c**: Wrapper for standard malloc/free symbols
- **econfig.h**: Configuration and constants

## License

[MIT License](LICENSE)
