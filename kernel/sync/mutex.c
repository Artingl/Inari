#include <kernel/inari.h>
#include <kernel/sync/mutex.h>
#include <kernel/proc/sched.h>

/* TODO: implement proper mutex with tasks's sleeping and waiting for it to release */

void mutex_init(mutex_t *lock)
{
    if (!lock) return;
    lock->lock = (atomic_flag)ATOMIC_FLAG_INIT;
}

void mutex_lock(mutex_t *lock)
{
    if (!lock) return;
    while (atomic_flag_test_and_set_explicit(&lock->lock, memory_order_acquire))
        sched_yield();
}

int mutex_test(mutex_t *lock)
{
    if (!lock) return 0;
    if (atomic_flag_test_and_set_explicit(&lock->lock, memory_order_acquire))
        return -1;
    atomic_flag_clear_explicit(&lock->lock, memory_order_release);
    return 0;
}

void mutex_unlock(mutex_t *lock)
{
    if (!lock) return;
    atomic_flag_clear_explicit(&lock->lock, memory_order_release);
}

