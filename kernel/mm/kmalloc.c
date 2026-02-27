#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/sync/spinlock.h>

#include <misc/string.h>

#define SMALL_ALLOC_THRESHOLD (PAGE_SIZE / 2)

static spinlock_t heap_lock = {0};
static struct block *heap_start = NULL;

int kmalloc_init(void) { return 0; }

void *kmalloc(size_t size) {
    uint32_t flags;
    spin_lock_irqsave(&heap_lock, flags);
    size = ALIGN(size + sizeof(struct block) + PAGE_SIZE, BLOCK_ALIGN);

    /* Large allocation: allocate whole pages */
    if (size > SMALL_ALLOC_THRESHOLD) {
        void *page = vmm_alloc_kernel((size + PAGE_SIZE - 1) >> 12);
        if (!page) {
            spin_unlock_irqrestore(&heap_lock, flags);
            return NULL;
        }

        struct block *b = (struct block *)page;
        b->size = (size + PAGE_SIZE - 1);
        b->free = 0;
        b->next = heap_start;
        b->large = 1;
        heap_start = b;
        spin_unlock_irqrestore(&heap_lock, flags);
        memset((void *)(b + 1), 0, size);
        return (void *)(b + 1);
    }

    /* Small allocation: try to reuse free blocks */
    struct block *curr = heap_start;
    if ((uintptr_t)curr < VIRTUAL_ADDR)
        panic("kmalloc: heap corrupted.");

    while (curr) {
        if (curr->free && !curr->large && curr->size >= size) {
            curr->free = 0;
            spin_unlock_irqrestore(&heap_lock, flags);
            memset((void *)(curr + 1), 0, size);
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }

    /* No free block found, allocate new page */
    void *page = vmm_alloc_kernel(1);
    if (!page) {
        spin_unlock_irqrestore(&heap_lock, flags);
        return NULL;
    }

    struct block *b = (struct block *)page;
    b->size = PAGE_SIZE - sizeof(struct block);
    b->free = 0;
    b->large = 0;
    b->next = heap_start;
    heap_start = b;
    spin_unlock_irqrestore(&heap_lock, flags);
    memset((void *)(b + 1), 0, size);
    return (void *)(b + 1);
}

void kfree(void *ptr) {
    if (!ptr)
        return;
    uint32_t flags;
    spin_lock_irqsave(&heap_lock, flags);
    struct block *b = ((struct block *)ptr) - 1;
    if (b->free)
        panic("kfree: double free!");
    b->free = 1;

    if (b->large) {
        /* Remove from list */
        struct block **prev = &heap_start;
        while (*prev && *prev != b)
            prev = &(*prev)->next;
        if (*prev)
            *prev = b->next;

        vmm_free_pages(arch_get_kernel_pagedir(), (void *)b, b->size >> 12);
    }
    spin_unlock_irqrestore(&heap_lock, flags);
}
