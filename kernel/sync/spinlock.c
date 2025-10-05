#include <kernel/sync/spinlock.h>

#include <arch/sys.h>

void spinlock_init(spinlock_t *lock)
{
    if (!lock) return;
    lock->lock = (atomic_flag)ATOMIC_FLAG_INIT;
}

void spin_lock(spinlock_t *lock)
{
    if (!lock) return;
    while (atomic_flag_test_and_set_explicit(&lock->lock, memory_order_acquire))
        ;
}

void spin_unlock(spinlock_t *lock)
{
    if (!lock) return;
    atomic_flag_clear_explicit(&lock->lock, memory_order_release);
}