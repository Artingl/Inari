#ifndef _INARI_PAGING_H
#define _INARI_PAGING_H

#include <misc/types.h>

#define TABLE_PRESENT (1 << 0)
#define TABLE_RW (1 << 1)

#define PAGE_PRESENT (1 << 0)
#define PAGE_RW (1 << 1)
#define PAGE_USR (1 << 2)
#define PAGE_DIRTY (1 << 5)

typedef void pagedir_t;

void *arch_virt_to_phys(pagedir_t *directory, void *vbase);
void *arch_map_page(pagedir_t *directory, void *vbase, void *pbase, size_t len, uint32_t flags);
void arch_unmap_page(pagedir_t *directory, void *vbase, size_t len);
void arch_switch_pagedir(pagedir_t *directory);
pagedir_t *arch_get_pagedir(void);
pagedir_t *arch_fork_pagedir(void);
pagedir_t *arch_get_kernel_pagedir();
void arch_free_pagedir(pagedir_t *directory);

#endif
