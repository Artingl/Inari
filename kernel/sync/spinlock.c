#include <kernel/inari.h>
#include <kernel/proc/sched.h>
#include <kernel/sync/spinlock.h>
#include <kernel/timer.h>

#include <arch/sys.h>

void spin_lock(spinlock_t *lock) {
    if (!lock)
        return;
#ifdef CONFIG_DEBUG
    size_t start_time = (timer_get_ticks() * 1000) / timer_get_resolution() * 1000;
#endif

    while (atomic_exchange_explicit(&lock->lock, 1, memory_order_acquire)) {
#ifdef CONFIG_DEBUG
        if (start_time > 10000000) {
            panic("spinlock timeout");
        }
#endif
    }
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
