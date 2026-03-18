#ifndef _LIBC_ALLOC_H
#define _LIBC_ALLOC_H

#include <types.h>

void *malloc(size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);

int atoi(const char *str);
long atol(const char *str);
double atof(const char *str);
long strtol(const char *str, char **endptr, int base);

__attribute__((unused)) static inline size_t align_up(uintptr_t value, size_t align) { return (value + align - 1) & ~(align - 1); }

#endif
