#include <kernel/inari.h>
#include <kernel/lock/spinlock.h>

// TODO: This is not SMP safe!
void spinlock_acquire(spinlock_t *lock)
{
    while (lock->lock)
        ;
    lock->lock = 1;
}

int spinlock_test(spinlock_t *lock)
{
    if (lock->lock)
        return -1;
    lock->lock = 1;
    return 0;
}

void spinlock_release(spinlock_t *lock)
{
    lock->lock = 0;
}

