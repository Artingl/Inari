#pragma once

typedef int atomic_t;

int atomic_set(atomic_t *atomic_t, uint32_t value);
int atomic_add(atomic_t *atomic_t);
int atomic_sub(atomic_t *atomic_t);
int atomic_get(atomic_t *atomic_t);
