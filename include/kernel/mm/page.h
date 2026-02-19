#ifndef _INARI_PAGE_H
#define _INARI_PAGE_H

#include <misc/types.h>

#define PAGE_RW         (1 << 0)
#define PAGE_USR        (1 << 1)
#define PAGE_PRESENT    (1 << 2)

typedef void* pagedir_t;

int page_init(void);

/* Map physical memory to virtual */
void *page_map(void *vbase, void *pbase, size_t len, uint32_t flags);
void page_unmap(void *vbase, size_t len);

/* Allocate N physical pages and map them in kernel space
 * Returns virtual address of mapped region
 */
void *page_alloc(size_t npages, uint32_t flags);
void page_free(void *vbase, size_t npages);

pagedir_t page_fork_dir(void);
pagedir_t page_get_dir(void);
void page_switch_dir(pagedir_t dir);
void page_dealloc_dir(pagedir_t dir);

uint8_t page_is_kernel_directory(void);

int page_is_kernel_pagedir();

#endif