#include <kernel/kernel.h>
#include <kernel/lock/spinlock.h>

#include <drivers/impl.h>

int spinlock_create(spinlock_t *lock)
{
    if (lock == NULL)
        return 1;
    lock->lock = 0;
    lock->count = 0;
    return 0;
}

int spinlock_acquire(spinlock_t *lock)
{
    if (lock == NULL)
        return 1;
    
    while (__sync_lock_test_and_set(&lock->lock, 1))
    {
        __asm__ volatile("pause":::"memory");
    }

    __disable_int();
    return 0;
}

int spinlock_release(spinlock_t *lock)
{
    if (lock == NULL)
        return 1;
    __sync_lock_release(&lock->lock);
    if (cpu_current_core()->ints_loaded)
        __enable_int();
    return 0;
}
