#ifndef _INARI_PMM_H
#define _INARI_PMM_H

#include <stddef.h>
#include <stdint.h>

#include <kernel/inari.h>

#define PMM_POOL_SIZE  262144

#define PMM_PAGE_AVAILABLE (1 << 0)
#define PMM_PAGE_DISABLED  (1 << 1)
#define PMM_PAGE_USED      (1 << 2)

typedef struct pmm_page {
    uint8_t flags;
} pmm_page_t;

extern int pmm_init(void);
extern void pmm_reserve_memory(struct reserved_memory region);
extern void *pmm_alloc_pages(size_t npages);
extern int pmm_free_pages(void *base, size_t npages);
extern size_t pmm_usage();
extern size_t pmm_total();

#endif