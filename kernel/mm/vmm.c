#include <kernel/inari.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>
#include <kernel/proc/sched.h>

#include <arch/paging.h>
#include <misc/string.h>

static struct vmm_page *pages_pool;
static uintptr_t vm_start;
extern char kern_phys_end;
extern char kern_virt_end;

static inline int vmm_mark_region(uintptr_t start, uintptr_t end, uint8_t flags)
{
    uintptr_t i;
    for (i = start; i < end; i += PAGE_SIZE)
    {
        if (!(pages_pool[i >> 12].flags & VMM_PAGE_RESERVED))
            pages_pool[i >> 12].flags = flags;
        // else return -1;
    }

    return 0;
}

int vmm_init(void)
{
    /* Allocate some memory for the VMM pages pool */
    pages_pool = arch_map_page(
        arch_get_kernel_pagedir(),
        (void*)VMM_VBASE,
        (void*)pmm_alloc_pages(VMM_SIZE_PAGES),
        VMM_SIZE_BYTES,
        PAGE_PRESENT | PAGE_RW);

    /* Mark the whole virtual memory as available at first */
    memset((void*)pages_pool, VMM_PAGE_AVAILABLE, VMM_SIZE_BYTES);
    
    /* Disable memory regions that are occupied by mapped kernel, or by identity mapping (below kern_phys_end) */
    vmm_mark_region(0, (uintptr_t)&kern_phys_end + PAGE_SIZE, VMM_PAGE_RESERVED);
    vmm_mark_region(VMM_VBASE, VMM_VBASE + VMM_SIZE_BYTES, VMM_PAGE_RESERVED);
    vmm_mark_region(VIRTUAL_ADDR, (uintptr_t)&kern_virt_end + PAGE_SIZE, VMM_PAGE_RESERVED);

    vm_start = (uintptr_t)&kern_phys_end + PAGE_SIZE;

    printk("vmm: initialized, base 0x%08x", pages_pool);
    return 0;
}

int vmm_check_flag(uintptr_t start, uintptr_t end, uint8_t flag)
{
    uintptr_t i;
    for (i = start; i < end; i += PAGE_SIZE)
        if (pages_pool[i >> 12].flags & flag) return -1;

    return 0;
}

/* TODO: vmm reserved memory MUST be pagedir specific IF not in kernel memory! */

int vmm_disable_region(struct reserved_memory region)
{
    return vmm_mark_region(region.start, region.end, VMM_PAGE_DISABLED);
}

int vmm_enable_region(struct reserved_memory region)
{
    return vmm_mark_region(region.start, region.end, VMM_PAGE_AVAILABLE);
}

static void *alloc_range(pagedir_t *target_dir, size_t npages, uintptr_t from_mem, uintptr_t to_mem, uint32_t flags)
{
    struct vmm_page *page;
    /* TODO: try to allocate physical memory in chunks, not contiguous */
    void *pbase = pmm_alloc_pages(npages);
    size_t block_offset = 0, block_size = 0, i;

    if (npages == 0 || !pbase)
        return NULL;

    /* Find free page in the pool */
    for (; from_mem < to_mem; from_mem += PAGE_SIZE)
    {
        page = &pages_pool[from_mem >> 12];
        if (!(page->flags & VMM_PAGE_AVAILABLE) || page->flags & VMM_PAGE_USED || page->flags & VMM_PAGE_DISABLED) {
            block_size = 0;
            continue;
        }

        if (block_size == 0)
            block_offset = from_mem >> 12;

        block_size++;
        if (block_size >= npages)
            break;
    }

    if (block_size < npages) {
        panic("vmm: no available contiguous blocks were found (npages = %d).", npages);
        return NULL;
    }

    /* Flag all blocks as used */
    for (i = 0; i < block_size; i++) {
        pages_pool[block_offset + i].flags |= VMM_PAGE_USED;
    }

    /* Map all the physical memory to virtual memory and return new vbase */
    return arch_map_page(target_dir, (void*)(block_offset * PAGE_SIZE), pbase, npages * PAGE_SIZE, flags);
}

void *vmm_alloc_user(pagedir_t *target_dir, size_t npages)
{
    return alloc_range(
        target_dir,
        npages,
        (uintptr_t)&kern_phys_end + PAGE_SIZE,
        VIRTUAL_ADDR - PAGE_SIZE,
        PAGE_PRESENT | PAGE_RW | PAGE_USR
    );
}

void *vmm_alloc_kernel(size_t npages)
{
    return alloc_range(
        arch_get_kernel_pagedir(),
        npages,
        VIRTUAL_ADDR,
        0xFFFFFFFF,
        PAGE_PRESENT | PAGE_RW
    );
}

static int free_pagedir(pagedir_t *target_dir, void *base, size_t npages)
{
    size_t i;
    if (base == NULL)
        return 1;

    for (i = 0; i < npages; i++)
    {
        void *pbase = arch_virt_to_phys(target_dir, base + i * PAGE_SIZE);
        if (pbase != NULL)
            pmm_free_pages(pbase, 1);
        pages_pool[((uintptr_t)base >> 12) + i].flags &= ~VMM_PAGE_USED;
    }

    arch_unmap_page(target_dir, base, npages);
    return 0;
}

int vmm_free_pages(pagedir_t *target_dir, void *base, size_t npages)
{
    return free_pagedir(
        target_dir,
        base, npages
    );
}
