#include <kernel/inari.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>
#include <arch/paging.h>

#include <misc/string.h>

#define _TABLE_PRESENT (1 << 0)
#define _TABLE_RW (1 << 1)

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_RW (1 << 1)
#define _PAGE_USR (1 << 2)
#define _PAGE_DIRTY (1 << 5)

struct page_table
{
    uint32_t pages[1024];
};

struct paging_directory
{
    uintptr_t tables_phys[1024];
};

struct paging_directory *x86_kernel_dir;
struct paging_directory *x86_current_dir;

#define align_up(size, align) (((size) + (align) - 1) & ~((align) - 1))

static inline void paging_flush_tlb(unsigned long addr)
{
   __asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline void *paging_alloc_table(struct paging_directory *dir, size_t offset)
{
    return (struct page_table *)(dir->tables_phys[offset] & ~0xFFF);
}

void *arch_virt_to_phys(void *vbase)
{
    struct page_table *table = paging_alloc_table(x86_current_dir, (uintptr_t)vbase >> 22);
    return (void*)((table->pages[(uintptr_t)vbase >> 12 & 0x03FF] & ~0xFFF)| ((uintptr_t)vbase & 0xFFF));
}

void *arch_map_page(void *vbase, void *pbase, size_t len, uint32_t flags)
{
    uintptr_t i, offset = (uintptr_t)pbase;
    uint32_t page_flags = 0;
    struct page_table *table = NULL;

    if (flags & PAGE_USR)
        page_flags |= _PAGE_USR;
    if (flags & PAGE_RW)
        page_flags |= _PAGE_RW;
    if (flags & PAGE_PRESENT)
        page_flags |= _PAGE_PRESENT;

    for (i = (uintptr_t)vbase; i < (uintptr_t)vbase + len; i+=PAGE_SIZE)
    {
        table = paging_alloc_table(x86_current_dir, i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)offset | page_flags;
        offset += PAGE_SIZE;
        paging_flush_tlb((uintptr_t)i);
    }

    return vbase;
}

void arch_unmap_page(void *vbase, size_t len)
{
    uintptr_t i;
    struct page_table *table = NULL;

    for (i = (uintptr_t)vbase; i < (uintptr_t)vbase + len; i+=PAGE_SIZE)
    {
        table = paging_alloc_table(x86_current_dir, i >> 22);
        table->pages[i >> 12 & 0x03FF] &= ~_PAGE_PRESENT;
    }
}

struct paging_directory *arch_get_pagedir(void)
{
    return x86_current_dir;
}

void arch_switch_pagedir(struct paging_directory *directory)
{
    if (!directory)
        return;
    
    __asm__ volatile("mov %0, %%cr3" ::"r"(arch_virt_to_phys((void*)directory)));
    x86_current_dir = directory;
}

struct paging_directory *arch_create_pagedir(void)
{
    size_t i;
    struct paging_directory *new_dir = (struct paging_directory*)kmalloc(sizeof(struct paging_directory) + PAGE_SIZE);
    new_dir = (struct paging_directory*)align_up((uintptr_t)new_dir, PAGE_SIZE);
    
    /* TODO: CoW */
    memcpy((void*)new_dir, (void*)x86_kernel_dir, sizeof(struct paging_directory));

    return new_dir;
}

void arch_cleanup_pagedir(struct paging_directory *directory)
{
    // if (!directory)
    //     kfree((void*)directory);

    printk("arch_cleanup_pagedir: not implemented");
}
