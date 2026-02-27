#include <kernel/proc/sched.h>
#include <kernel/sync/spinlock.h>

#include <arch/sys.h>

void spin_lock(spinlock_t *lock) {
    if (!lock)
        return;
    while (atomic_exchange_explicit(&lock->lock, 1, memory_order_acquire))
        ;
    // sched_yield();
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