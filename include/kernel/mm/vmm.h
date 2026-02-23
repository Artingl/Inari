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

extern char vmm_pages_pool;

#define VMM_SIZE_BYTES   (sizeof(struct vmm_page) * (0xffffffff >> 12) + PAGE_SIZE)
#define VMM_SIZE_PAGES   (VMM_SIZE_BYTES >> 12)
#define VMM_VBASE        ((uintptr_t)&vmm_pages_pool)

int vmm_init(void);
void *vmm_alloc_user(pagedir_t *target_dir, size_t npages);
void *vmm_alloc_kernel(size_t npages);
int vmm_free_pages(pagedir_t *target_dir, void *base, size_t npages);
int vmm_disable_region(struct reserved_memory region);
int vmm_enable_region(struct reserved_memory region);
int vmm_check_flag(uintptr_t start, uintptr_t end, uint8_t flag);

#endif