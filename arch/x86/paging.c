#include <kernel/inari.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/kmalloc.h>
#include <arch/paging.h>

#include <arch/x86/cpu.h>
#include <arch/paging.h>
#include <misc/string.h>

struct x86_paging_directory *x86_kernel_dir;
struct x86_paging_directory *x86_current_dir;

static inline void flush_tlb(unsigned long addr)
{
   __asm__ volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline struct x86_page_table *get_table(struct x86_paging_directory *dir, size_t offset)
{
    return (struct x86_page_table *)(dir->tables_virt[offset] & ~0xFFF);
}

void *arch_virt_to_phys(pagedir_t *directory, void *vbase)
{
    struct x86_page_table *table = get_table((struct x86_paging_directory*)directory, (uintptr_t)vbase >> 22);
    return (void*)((table->pages[(uintptr_t)vbase >> 12 & 0x03FF] & ~0xFFF)| ((uintptr_t)vbase & 0xFFF));
}

void *arch_map_page(pagedir_t *directory, void *vbase, void *pbase, size_t len, uint32_t flags)
{
    uintptr_t i, offset = (uintptr_t)pbase;
    struct x86_page_table *table = NULL;

    for (i = (uintptr_t)vbase; i < (uintptr_t)vbase + len; i+=PAGE_SIZE)
    {
        table = get_table((struct x86_paging_directory*)directory, i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)offset | flags;
        offset += PAGE_SIZE;
        if (directory == (pagedir_t*)x86_current_dir || i >= VIRTUAL_ADDR || i <= 0x1000000)
            flush_tlb((uintptr_t)i);
    }

    return vbase;
}

void arch_unmap_page(pagedir_t *directory, void *vbase, size_t len)
{
    uintptr_t i;
    struct x86_page_table *table = NULL;

    for (i = (uintptr_t)vbase; i < (uintptr_t)vbase + len; i+=PAGE_SIZE)
    {
        table = get_table((struct x86_paging_directory*)directory, i >> 22);
        table->pages[i >> 12 & 0x03FF] &= ~PAGE_PRESENT;
        table->pages[i >> 12 & 0x03FF] &= ~PAGE_USR;
        table->pages[i >> 12 & 0x03FF] |= PAGE_DIRTY;
        if (directory == (pagedir_t*)x86_current_dir)
            flush_tlb((uintptr_t)i);
    }
}

pagedir_t *arch_get_pagedir(void)
{
    return (pagedir_t*)x86_current_dir;
}

void arch_switch_pagedir(pagedir_t *directory)
{
    if (!directory)
        return;
    
    __asm__ volatile("mov %0, %%cr3" ::"r"(arch_virt_to_phys((pagedir_t*)x86_current_dir, (void*)directory)));
    x86_current_dir = (struct x86_paging_directory*)directory;
}

pagedir_t *arch_fork_pagedir(void)
{
    size_t i;
    struct x86_paging_directory_usr *new_dir = (struct x86_paging_directory_usr*)vmm_alloc_kernel((sizeof(struct x86_paging_directory_usr) >> 12) + 1);
    memset((void*)new_dir, 0, sizeof(struct x86_paging_directory_usr));
    
    /* TODO: Dynamic allocation */
    for (i = 0; i < 1024; i++)
    {
        if (i >= 768 || i < 4)
        {
            new_dir->dir.tables_phys[i] = x86_kernel_dir->tables_phys[i];
            new_dir->dir.tables_virt[i] = x86_kernel_dir->tables_virt[i];
        }
        else
        {
            struct x86_page_table *new_table = &new_dir->tables_pool[i];
            new_table = (struct x86_page_table*)ALIGN((uintptr_t)new_table, PAGE_SIZE);
            new_dir->dir.tables_virt[i] = (uintptr_t)new_table | TABLE_PRESENT | TABLE_RW | TABLE_USR;
            new_dir->dir.tables_phys[i] = (uintptr_t)arch_virt_to_phys(x86_kernel_dir, (void*)new_dir->dir.tables_virt[i]) | TABLE_PRESENT | TABLE_RW | TABLE_USR;
        }
    }

    vmm_init_directory((pagedir_t*)&new_dir->dir);

    return (pagedir_t*)&new_dir->dir;
}

void arch_free_pagedir(pagedir_t *directory)
{
    if (!directory)
        return;
    
    /* Cleanup all allocated resources, vmm will do it for us */
    vmm_cleanup_directory(directory);

    /* Finally, deallocate it; Also switch to kernel directory just in case */
    arch_switch_pagedir(arch_get_kernel_pagedir());
    vmm_free_pages(arch_get_kernel_pagedir(), (void*)directory, (sizeof(struct x86_paging_directory_usr) >> 12) + 1);
}

pagedir_t *arch_get_kernel_pagedir()
{
    return (pagedir_t*)x86_kernel_dir;
}