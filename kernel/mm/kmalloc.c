#include <kernel/inari.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/kmalloc.h>

#define SMALL_ALLOC_THRESHOLD (PAGE_SIZE / 2)

static struct block *heap_start = NULL;

int kmalloc_init(void)
{
    return 0;
}

void *kmalloc(size_t size)
{
    size = ALIGN(size + PAGE_SIZE, BLOCK_ALIGN);

    /* Large allocation: allocate whole pages */
    if (size > SMALL_ALLOC_THRESHOLD)
    {
        size_t npages = (size + PAGE_SIZE - 1) >> 12;
        void *page = vmm_alloc_kernel(npages);
        if (!page) return NULL;

        struct block *b = (struct block*)page;
        b->size = npages * PAGE_SIZE;
        b->free = 0;
        b->next = heap_start;
        b->large = 1;
        heap_start = b;
        return (void*)(b + 1);
    }

    /* Small allocation: try to reuse free blocks */
    struct block *curr = heap_start;
    while (curr)
    {
        if (curr->free && !curr->large && curr->size >= size)
        {
            curr->free = 0;
            return (void*)(curr + 1);
        }
        curr = curr->next;
    }

    /* No free block found, allocate new page */
    void *page = vmm_alloc_kernel(1);
    if (!page) return NULL;

    struct block *b = (struct block*)page;
    b->size = PAGE_SIZE - sizeof(struct block);
    b->free = 0;
    b->large = 0;
    b->next = heap_start;
    heap_start = b;
    return (void*)(b + 1);
}

void kfree(void *ptr)
{
    if (!ptr) return;
    struct block *b = ((struct block*)ptr) - 1;
    b->free = 1;

    if (b->large)
    {
        /* Remove from list */
        struct block* *prev = &heap_start;
        while (*prev && *prev != b) prev = &(*prev)->next;
        if (*prev) *prev = b->next;

        vmm_free_pages(arch_get_kernel_pagedir(), (void*)b, b->size >> 12);
    }
}
