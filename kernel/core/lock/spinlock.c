#include <kernel/kernel.h>
#include <kernel/core/lock/spinlock.h>

#include <kernel/arch/i686/impl.h>

int spinlock_init(spinlock_t *lock)
{
    if (lock == NULL)
        return -1;
    lock->lock = 0;
    lock->count = 0;
    lock->owner_tid = 0;
    return 0;
}

int spinlock_acquire(spinlock_t *lock)
{
    if (lock == NULL)
        return 1;
    
    // scheduler_task_t *task = scheduler_current();
    // size_t current_tid = 0;
    // if (task != NULL)
    //     current_tid = task->tid;
    
    // lock->enable_interrupts = cpu_current_core()->ints_loaded && __eint();
    // __disable_int();


    while (__sync_lock_test_and_set(&lock->lock, 1))
    {
        __asm__ volatile("pause":::"memory");
    }
    lock->lock = 1;

    return 0;
}

int spinlock_release(spinlock_t *lock)
{
    if (lock == NULL)
        return 1;

    __sync_lock_release(&lock->lock);
    // if (lock->enable_interrupts)
    //     __enable_int();
    // lock->owner_tid = 0;
    // lock->enable_interrupts = 0;
    return 0;
}

int spinlock_trylock(spinlock_t *lock)
{

    // int flip_interrupts = cpu_current_core()->ints_loaded && __eint();
    if (lock == NULL)
        return 1;
    
    if (!lock->lock) lock->lock = 1;
    else return 1;
    // if (flip_interrupts)
    //     __disable_int();
    // if (__sync_lock_test_and_set(&lock->lock, 1))
    //     return 1;
    // if (flip_interrupts)
    //     __enable_int();
    return 0;
}
