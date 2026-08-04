#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ALLOCATIONS 100000

void* allocations[MAX_ALLOCATIONS];
size_t allocation_sizes[MAX_ALLOCATIONS];

uint32_t generate_seed(int alloc_id, int offset)
{
    uint32_t x = (uint32_t)alloc_id * 2654435761U + offset;
    x ^= x >> 15;
    x *= 2246822519U;
    x ^= x >> 13;
    return x;
}

void fill_allocation(void* ptr, size_t size, int alloc_id)
{
    uint32_t* data = (uint32_t*)ptr;
    size_t full_words = size / sizeof(uint32_t);
    for (size_t i = 0; i < full_words; i++)
    {
        data[i] = generate_seed(alloc_id, i);
    }

    uint8_t* remaining = (uint8_t*)(data + full_words);
    for (size_t i = 0; i < size % sizeof(uint32_t); i++)
    {
        remaining[i] = (uint8_t)(generate_seed(alloc_id, full_words) >> (i * 8));
    }
}

int verify_allocation(void* ptr, size_t size, int alloc_id)
{
    uint32_t* data = (uint32_t*)ptr;
    size_t full_words = size / sizeof(uint32_t);
    for (size_t i = 0; i < full_words; i++)
    {
        if (data[i] != generate_seed(alloc_id, i))
        {
            return 0;
        }
    }

    uint8_t* remaining = (uint8_t*)(data + full_words);
    for (size_t i = 0; i < size % sizeof(uint32_t); i++)
    {
        if (remaining[i] != (uint8_t)(generate_seed(alloc_id, full_words) >> (i * 8)))
        {
            return 0;
        }
    }
    return 1;
}

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <allocator so> <trace_file>\n", argv[0]);
        return 1;
    }

    void* allocator_lib = dlopen(argv[1], RTLD_LAZY);
    if (!allocator_lib)
    {
        fprintf(stderr, "Failed to open the allocator library\n");
        return 1;
    }

    void* (*lib_malloc)(size_t size) = dlsym(allocator_lib, "malloc");
    const char* err = dlerror();
    if (err)
    {
        fprintf(stderr, "dlsym while loading malloc: %s\n", err);
    }

    void (*lib_free)(void* ptr) = dlsym(allocator_lib, "free");
    err = dlerror();
    if (err)
    {
        fprintf(stderr, "dlsym while loading free: %s\n", err);
    }

    FILE* fp = fopen(argv[2], "r");
    if (!fp)
    {
        perror("fopen");
        return 1;
    }

    int total_ops, num_allocs, num_frees;
    if (fscanf(fp, "%d %d %d", &total_ops, &num_allocs, &num_frees) != 3)
    {
        fprintf(stderr, "Error reading trace header\n");
        fclose(fp);
        return 1;
    }

    printf("Trace: total_ops=%d, allocs=%d, frees=%d\n", total_ops, num_allocs, num_frees);

    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
        allocations[i] = NULL;
        allocation_sizes[i] = 0;
    }

    int op_count = 0;
    char op_type;
    int operation_index, size;

    while (fscanf(fp, " %c", &op_type) == 1)
    {
        if (op_type == 'a')
        {
            if (fscanf(fp, "%d %d", &operation_index, &size) != 2)
            {
                fprintf(stderr, "Error parsing allocate operation\n");
                break;
            }

            if (operation_index >= MAX_ALLOCATIONS)
            {
                fprintf(stderr, "Allocation id %d exceeds MAX_ALLOCATIONS\n", operation_index);
                return 1;
            }

            allocations[operation_index] = lib_malloc(size);
            if (!allocations[operation_index])
            {
                fprintf(stderr, "malloc(%d) failed\n", size);
                return 1;
            }
            allocation_sizes[operation_index] = size;
            fill_allocation(allocations[operation_index], size, operation_index);
            op_count++;
        }

        else if (op_type == 'f')
        {
            if (fscanf(fp, "%d", &operation_index) != 1)
            {
                fprintf(stderr, "Error parsing free operation\n");
                break;
            }

            if (operation_index >= MAX_ALLOCATIONS)
            {
                fprintf(stderr, "Allocation id %d exceeds MAX_ALLOCATIONS\n", operation_index);
                return 1;
            }

            if (allocations[operation_index] == NULL)
            {
                fprintf(stderr, "Freeing already-freed or unallocated block %d\n", operation_index);
                return 1;
            }

            if (!verify_allocation(allocations[operation_index], allocation_sizes[operation_index],
                    operation_index))
            {
                fprintf(stderr, "Data integrity check failed for allocation %d (size %zu)\n",
                    operation_index, allocation_sizes[operation_index]);
                return 1;
            }

            lib_free(allocations[operation_index]);
            allocations[operation_index] = NULL;
            allocation_sizes[operation_index] = 0;
            op_count++;
        }
        else
        {
            fprintf(stderr, "Unknown operation type: %c\n", op_type);
            return 1;
        }
    }

    fclose(fp);

    printf("Successfully executed %d operations\n", op_count);

    int integrity_failures = 0;
    for (int i = 0; i < MAX_ALLOCATIONS; i++)
    {
        if (allocations[i] != NULL)
        {
            if (!verify_allocation(allocations[i], allocation_sizes[i], i))
            {
                fprintf(stderr,
                    "Data integrity check failed for allocation %d (size %zu) at exit\n", i,
                    allocation_sizes[i]);
                integrity_failures++;
            }
            else
            {
                printf("Verified allocation %d (size %zu) at exit\n", i, allocation_sizes[i]);
            }
        }
    }

    if (integrity_failures > 0)
    {
        fprintf(
            stderr, "%d allocations failed data integrity checks at exit\n", integrity_failures);
        return 1;
    }

    return 0;
}
