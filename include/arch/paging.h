#ifndef _INARI_PAGING_H
#define _INARI_PAGING_H

#include <misc/types.h>
#include <kernel/mm/page.h>

#define _TABLE_PRESENT (1 << 0)
#define _TABLE_RW (1 << 1)

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_RW (1 << 1)
#define _PAGE_USR (1 << 2)
#define _PAGE_DIRTY (1 << 5)

struct page_table
{
    uint32_t pages[1024];
} __attribute__((packed));

struct paging_directory
{
    uintptr_t tables_phys[1024];
    uintptr_t tables_virt[1024];
    uint8_t is_kernel;
} __attribute__((packed));

struct paging_directory_usr
{
    struct paging_directory dir;
    struct page_table tables_pool[1024];
} __attribute__((packed));

void *arch_virt_to_phys(void *vbase);
void *arch_map_page(void *vbase, void *pbase, size_t len, uint32_t flags);
void arch_unmap_page(void *vbase, size_t len);
void arch_switch_pagedir(struct paging_directory *directory);
struct paging_directory *arch_get_pagedir(void);
struct paging_directory *arch_create_pagedir(void);
void arch_cleanup_pagedir(struct paging_directory *directory);
int arch_page_is_in_kernel();

#endif
