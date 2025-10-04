#ifndef _INARI_SPINLOCK_H
#define _INARI_SPINLOCK_H

#include <stddef.h>
#include <stdint.h>

typedef struct spinlock
{
    uint8_t lock;
} spinlock_t;

extern void spinlock_acquire(spinlock_t *lock);
extern int spinlock_test(spinlock_t *lock);
extern void spinlock_release(spinlock_t *lock);

#endif