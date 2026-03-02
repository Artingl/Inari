#ifndef _INARI_PMM_H
#define _INARI_PMM_H

#include <kernel/inari.h>
#include <misc/types.h>

#define PMM_POOL_SIZE 1048576

#define PMM_PAGE_AVAILABLE (1 << 0)
#define PMM_PAGE_DISABLED  (1 << 1)
#define PMM_PAGE_USED      (1 << 2)

typedef struct pmm_page {
    uint8_t flags;
} pmm_page_t;

int pmm_init(void);
void pmm_reserve_memory(struct reserved_memory region);
void *pmm_alloc_pages(size_t npages);
int pmm_free_pages(void *base, size_t npages);
size_t pmm_usage();
size_t pmm_total();

#endif