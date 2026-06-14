#ifndef _INARI_SPINLOCK_H
#define _INARI_SPINLOCK_H

#include <arch/sys.h>
#include <misc/types.h>

#include <stdatomic.h>

#define spin_lock_irqsave(lock, flags)                                                                                 \
    do {                                                                                                               \
        local_irq_save(flags);                                                                                         \
        spin_lock(lock);                                                                                               \
    } while (0)

#define spin_unlock_irqrestore(lock, flags)                                                                            \
    do {                                                                                                               \
        spin_unlock(lock);                                                                                             \
        local_irq_restore(flags);                                                                                      \
    } while (0)

typedef struct spinlock {
    atomic_bool lock;
} spinlock_t;

int spin_lock_is_free(spinlock_t *lock);
int spin_try_lock(spinlock_t *lock); // returns 0 if not locked, 1 if locked
void spin_lock(spinlock_t *lock);
void spin_unlock(spinlock_t *lock);

#endif
