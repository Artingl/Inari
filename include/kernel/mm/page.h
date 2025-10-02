#ifndef _INARI_PAGE_H
#define _INARI_PAGE_H

#include <stddef.h>
#include <stdint.h>

#define PAGE_RW         (1 << 0)
#define PAGE_USR        (1 << 1)
#define PAGE_PRESENT    (1 << 2)

extern int page_init(void);

// Map physical memory to virtual
extern void *page_map(void *vbase, void *pbase, size_t len, uint32_t flags);
extern void page_unmap(void *vbase, size_t len);

// Allocate N physical pages and map them in kernel space
// Returns virtual address of mapped region 
extern void *page_alloc(size_t npages, uint32_t flags);
extern void page_free(void *vbase, size_t npages);


#endif