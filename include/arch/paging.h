#ifndef _INARI_PAGING_H
#define _INARI_PAGING_H

#include <misc/types.h>

struct paging_directory;

void *arch_virt_to_phys(void *vbase);
void *arch_map_page(void *vbase, void *pbase, size_t len, uint32_t flags);
void arch_unmap_page(void *vbase, size_t len);
void arch_switch_pagedir(struct paging_directory *directory);
struct paging_directory *arch_get_pagedir(void);
struct paging_directory *arch_create_pagedir(void);
void arch_cleanup_pagedir(struct paging_directory *directory);

#endif
