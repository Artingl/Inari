#pragma once

#include <stdatomic.h>

#include <drivers/cpu/cpu.h>

typedef struct spinlock {
    volatile uint8_t lock;
    uint32_t count;
} spinlock_t;

int spinlock_create(spinlock_t *lock);
int spinlock_acquire(spinlock_t *lock);
int spinlock_release(spinlock_t *lock);
