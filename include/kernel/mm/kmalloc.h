#ifndef _INARI_KMALLOC_H
#define _INARI_KMALLOC_H

#define BLOCK_ALIGN 8

#include <misc/types.h>

struct block {
    size_t size;
    struct block *next;
    int free;
    int large; // 1 if this block spans multiple pages
};

int kmalloc_init(void);

void *kmalloc(size_t size);
void kfree(void *ptr);

#endif
