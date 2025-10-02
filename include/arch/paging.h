#ifndef _INARI_PAGING_H
#define _INARI_PAGING_H

#include <stddef.h>
#include <stdint.h>

struct paging_directory;

extern void *arch_phys_page(void *vbase);
extern void *arch_map_page(void *vbase, void *pbase, size_t len, uint32_t flags);
extern void arch_unmap_page(void *vbase, size_t len);
extern void arch_switch_pagedir(struct paging_directory *directory);
extern struct paging_directory *arch_get_pagedir(void);

#endif
