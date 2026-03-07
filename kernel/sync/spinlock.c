#include <kernel/proc/sched.h>
#include <kernel/sync/spinlock.h>

#include <arch/sys.h>

void spin_lock(spinlock_t *lock) {
    if (!lock)
        return;
    while (atomic_exchange_explicit(&lock->lock, 1, memory_order_acquire))
        ;
}

int spin_try_lock(spinlock_t *lock) {
    if (!lock)
        return 0;
    uint8_t expected = 0;
    return atomic_compare_exchange_strong(&lock->lock, &expected, 1);
}

int spin_lock_is_free(spinlock_t *lock) {
    if (!lock)
        return 1;
    return !atomic_load_explicit(&lock->lock, memory_order_relaxed);
}

void spin_unlock(spinlock_t *lock) {
    if (!lock)
        return;
    atomic_store_explicit(&lock->lock, 0, memory_order_release);
}
