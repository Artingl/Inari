#include <io.h>
#include <lib.h>
#include <string.h>
#include <sys.h>

#include <types.h>

struct block {
    size_t size, real_size;
    struct block *next;
    int free;
    int large; // 1 if this block spans multiple pages
};

#define BLOCK_ALIGN           8
#define PAGE_SIZE             0x1000
#define SMALL_ALLOC_THRESHOLD (PAGE_SIZE / 2)

static struct block *heap_start = NULL;
static inline size_t align_up(size_t size, size_t align) { return (size + align - 1) & ~(align - 1); }

void *realloc(void *ptr, size_t size) {
    if (!ptr)
        return malloc(size);

    if (size == 0) {
        free(ptr);
        return NULL;
    }

    struct block *b = ((struct block *)ptr) - 1;

    if (b->size >= size) {
        b->real_size = size;
        return ptr;
    }

    void *new_ptr = malloc(size);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, b->real_size);

    free(ptr);
    return new_ptr;
}

void *malloc(size_t real_size) {
    size_t size = align_up(real_size + PAGE_SIZE, BLOCK_ALIGN);

    /* Large allocation: allocate whole pages */
    if (size > SMALL_ALLOC_THRESHOLD) {
        size_t npages = (size + PAGE_SIZE - 1) >> 12;
        void *page = memalloc(npages, MEM_USR | MEM_PRESENT | MEM_RW);
        if (!page)
            return NULL;

        struct block *b = (struct block *)page;
        b->size = npages * PAGE_SIZE;
        b->real_size = real_size;
        b->free = 0;
        b->next = heap_start;
        b->large = 1;
        heap_start = b;
        return (void *)(b + 1);
    }

    /* Small allocation: try to reuse free blocks */
    struct block *curr = heap_start;
    while (curr) {
        if (curr->free && !curr->large && curr->size >= size) {
            curr->free = 0;
            return (void *)(curr + 1);
        }
        curr = curr->next;
    }

    /* No free block found, allocate new page */
    void *page = memalloc(1, MEM_USR | MEM_PRESENT | MEM_RW);
    if (!page)
        return NULL;

    struct block *b = (struct block *)page;
    b->size = PAGE_SIZE - sizeof(struct block);
    b->real_size = real_size;
    b->free = 0;
    b->large = 0;
    b->next = heap_start;
    heap_start = b;
    return (void *)(b + 1);
}

void free(void *ptr) {
    if (!ptr)
        return;
    struct block *b = ((struct block *)ptr) - 1;
    if (b->free) {
        printf("alloc: double free.\n");
        exit(-1);
    }
    b->free = 1;

    if (b->large) {
        /* Remove from list */
        struct block **prev = &heap_start;
        while (*prev && *prev != b)
            prev = &(*prev)->next;
        if (*prev)
            *prev = b->next;

        memfree((void *)b, b->size >> 12);
    }
}
