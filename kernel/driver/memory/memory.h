#pragma once

#include <kernel/include/C/typedefs.h>
#include <kernel/kernel.h>

// These functions must be implemented inside the memory driver of currently booted architecture

void kident(void *addr, size_t length, uint32_t flags);
void kunident(void *addr, size_t length);

int kmmap(
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags);

void *kmalloc(size_t length);
void *krealloc(void *ptr, size_t size);
void *kcalloc(size_t n, size_t size);
void kfree(void *ptr);

void memory_info();

// Forbid the memory driver to allocate a region of physical memory at origin
int memory_forbid_region(uintptr_t origin, size_t size);
