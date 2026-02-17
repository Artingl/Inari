#include <kernel/inari.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>
#include <kernel/mm/kmalloc.h>
#include <arch/paging.h>

#include <misc/string.h>

struct paging_directory *x86_kernel_dir;
struct paging_directory *x86_current_dir;

static inline void flush_tlb(unsigned long addr)
{
   __asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline void *get_table(struct paging_directory *dir, size_t offset)
{
    return (struct page_table *)(dir->tables_virt[offset] & ~0xFFF);
}

static struct page_table *copy_table(struct page_table *table)
{
    struct page_table *new_table = (struct page_table*)kmalloc(sizeof(struct page_table) + PAGE_SIZE);
    new_table = (struct page_table*)ALIGN((uintptr_t)new_table, PAGE_SIZE);
    memcpy((void*)new_table, (void*)table, sizeof(struct page_table));
    return new_table;
}

void *arch_virt_to_phys(void *vbase)
{
    struct page_table *table = get_table(x86_current_dir, (uintptr_t)vbase >> 22);
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
        table = get_table(x86_current_dir, i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)offset | page_flags;
        offset += PAGE_SIZE;
        flush_tlb((uintptr_t)i);
    }

    return vbase;
}

void arch_unmap_page(void *vbase, size_t len)
{
    uintptr_t i;
    struct page_table *table = NULL;

    for (i = (uintptr_t)vbase; i < (uintptr_t)vbase + len; i+=PAGE_SIZE)
    {
        table = get_table(x86_current_dir, i >> 22);
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
    new_dir = (struct paging_directory*)ALIGN((uintptr_t)new_dir, PAGE_SIZE);
    
    /* TODO: CoW */
    for (i = 0; i < 1024; i++)
    {
        if ((i << 22) > VIRTUAL_ADDR || (i << 22) < 0x300000)
            new_dir->tables_virt[i] = ((uintptr_t)copy_table((struct page_table*)(x86_kernel_dir->tables_phys[i] & ~0xFFF))) | _TABLE_PRESENT | _TABLE_RW;
        else
        {
            struct page_table *new_table = (struct page_table*)kmalloc(sizeof(struct page_table) + PAGE_SIZE);
            new_table = (struct page_table*)ALIGN((uintptr_t)new_table, PAGE_SIZE);
            new_dir->tables_virt[i] = (uintptr_t)new_table | _TABLE_PRESENT | _TABLE_RW;
        }

        new_dir->tables_phys[i] = (uintptr_t)arch_virt_to_phys((void*)new_dir->tables_virt[i]);
    }

    return new_dir;
}

void arch_cleanup_pagedir(struct paging_directory *directory)
{
    // if (!directory)
    //     kfree((void*)directory);

    printk("arch_cleanup_pagedir: not implemented");
}

struct paging_window *arch_get_window()
{
    return &x86_current_dir->window;
}
