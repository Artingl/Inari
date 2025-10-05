#ifndef _INARI_SPINLOCK_H
#define _INARI_SPINLOCK_H

#include <misc/types.h>
#include <arch/sys.h>

#include <stdatomic.h>

#define spin_lock_irqsave(lock, flags)     \
    do {                                   \
        local_irq_save(flags);             \
        spin_lock(lock);                   \
    } while (0)

#define spin_unlock_irqrestore(lock, flags) \
    do {                                    \
        spin_unlock(lock);                  \
        local_irq_restore(flags);           \
    } while (0)

typedef struct spinlock
{
    atomic_flag lock;
} spinlock_t;

extern void spinlock_init(spinlock_t *lock);
extern void spin_lock(spinlock_t *lock);
extern void spin_unlock(spinlock_t *lock);


#endif