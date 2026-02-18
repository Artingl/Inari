#include <kernel/inari.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>

#define SMALL_ALLOC_THRESHOLD (PAGE_SIZE / 2)

static struct block *heap_start = NULL;
static inline size_t align_up(size_t size, size_t align)
{
    return (size + align - 1) & ~(align - 1);
}

int kmalloc_init(void)
{
    return 0;
}

void *kmalloc(size_t size)
{
    size = align_up(size + PAGE_SIZE, BLOCK_ALIGN);

    /* Large allocation: allocate whole pages */
    if (size > SMALL_ALLOC_THRESHOLD)
    {
        size_t npages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
        void *page = page_alloc(npages, PAGE_PRESENT | PAGE_RW);
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
    void *page = page_alloc(1, PAGE_PRESENT | PAGE_RW);
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

        page_free((void*)b, b->size / PAGE_SIZE);
    }
}
