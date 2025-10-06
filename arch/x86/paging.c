#include <kernel/inari.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>
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

static inline struct paging_directory *paging_get_pagedir(void)
{
    uintptr_t cr3;
    __asm__ volatile ("mov %%cr3, %0" : "=r"(cr3));
    return (struct paging_directory*)cr3;
}

static inline void paging_flush_tlb(unsigned long addr)
{
   __asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline void *paging_alloc_table(size_t offset)
{
    struct paging_directory *dir = paging_get_pagedir();
    return (struct page_table *)(dir->tables_phys[offset] & ~0xFFF);
}

void *arch_phys_page(void *vbase)
{
    struct page_table *table = paging_alloc_table((uintptr_t)vbase >> 22);
    return (void*)(table->pages[(uintptr_t)vbase >> 12 & 0x03FF] & ~0xFFF);
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
        table = paging_alloc_table(i >> 22);
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
        table = paging_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] &= ~_PAGE_PRESENT;
    }
}

struct paging_directory *arch_get_pagedir(void)
{
    return paging_get_pagedir();
}

void arch_switch_pagedir(struct paging_directory *directory)
{
    if (!directory)
        return;

    panic("not implemented arch_switch_pagedir");
    // uint32_t cr0;
    // __asm__ volatile("mov %0, %%cr3" ::"r"(&directory->tables_phys[0]));
    // __asm__ volatile("mov %%cr0, %0"
    //                  : "=r"(cr0));
    // cr0 |= 0x80000000;
    // __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}
