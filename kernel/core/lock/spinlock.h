#pragma once

#include <stdatomic.h>

#include <kernel/libc/typedefs.h>

typedef struct spinlock {
    volatile uint8_t lock;
    uint32_t count;
    size_t owner_tid;
} spinlock_t;

int spinlock_init(spinlock_t *lock);
int spinlock_acquire(spinlock_t *lock);
int spinlock_trylock(spinlock_t *lock);
int spinlock_release(spinlock_t *lock);
