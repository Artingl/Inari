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

extern int kmalloc_init(void);

extern void *kmalloc(size_t size);
extern void kfree(void *ptr);

#endif