#include "emalloc.h"

void spin_lock(spinlock_t* lock)
{
#ifdef DEBUG_LOG_STUCK_SPINLOCKS
    size_t spins = 0;
#endif

    while (atomic_flag_test_and_set_explicit(&lock->locked, memory_order_acquire))
    {
        // Do nothing
#ifdef DEBUG_LOG_STUCK_SPINLOCKS
        if (++spins > 100000000)
        {
            panic("spinlock: stuck spinlock\n");
        }
#endif
    }
}

void spin_unlock(spinlock_t* lock)
{
    atomic_flag_clear_explicit(&lock->locked, memory_order_release);
}