#ifndef _INARI_VMM_H
#define _INARI_VMM_H

#include <misc/types.h>
#include <kernel/inari.h>

#define VMM_PAGE_AVAILABLE     (1 << 0)
#define VMM_PAGE_DISABLED      (1 << 1)
#define VMM_PAGE_RESERVED      (1 << 2)
#define VMM_PAGE_USED          (1 << 3)

typedef struct vmm_page {
    uint8_t flags;
} vmm_page_t;

#define VMM_SIZE_BYTES   (sizeof(struct vmm_page) * (0xffffffff / PAGE_SIZE))
#define VMM_SIZE_PAGES   (VMM_SIZE_BYTES / PAGE_SIZE)
#define VMM_VBASE        (VIRTUAL_ADDR - VMM_SIZE_BYTES)

int vmm_init(void);
void vmm_in_kernel(uint8_t flag);
void *vmm_alloc_pages(size_t npages);
int vmm_free_pages(void *base, size_t npages);
int vmm_disable_region(struct reserved_memory region);
int vmm_enable_region(struct reserved_memory region);

#endif