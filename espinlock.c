#include "emalloc.h"

void spin_lock(spinlock_t* lock)
{
    while (atomic_flag_test_and_set_explicit(&lock->locked, memory_order_acquire))
    {
        // Do nothing
    }
}

void spin_unlock(spinlock_t* lock)
{
    atomic_flag_clear_explicit(&lock->locked, memory_order_release);
}