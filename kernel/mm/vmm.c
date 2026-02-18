#include <kernel/inari.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>

#include <arch/paging.h>
#include <misc/string.h>

static struct vmm_page *pages_pool;
static uintptr_t vm_start;
extern char kern_phys_end;
extern char kern_virt_end;

static inline void vmm_mark_region(uintptr_t start, uintptr_t end, uint8_t flags)
{
    uintptr_t i;
    for (i = start; i < end; i += PAGE_SIZE)
    {
        if (!(pages_pool[i / PAGE_SIZE].flags & VMM_PAGE_RESERVED))
            pages_pool[i / PAGE_SIZE].flags = flags;
    }
}

int vmm_init(void)
{
    /* Allocate some memory for the VMM pages pool */
    pages_pool = arch_map_page(
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
        if (pages_pool[i / PAGE_SIZE].flags & flag) return -1;

    return 0;
}

int vmm_disable_region(struct reserved_memory region)
{
    vmm_mark_region(region.start, region.end, VMM_PAGE_DISABLED);
    return 0;
}

int vmm_enable_region(struct reserved_memory region)
{
    vmm_mark_region(region.start, region.end, VMM_PAGE_AVAILABLE);
    return 0;
}

void *vmm_alloc_pages(size_t npages)
{
    struct vmm_page *page;
    size_t block_offset = 0, block_size = 0;
    uintptr_t i, end_addr;

    if (npages == 0)
        return NULL;

    /* Allocate in different VM space if not allocating for kernel */
    if (page_is_in_kernel_glbl() && page_is_in_kernel())
    {
        i = VIRTUAL_ADDR;
        end_addr = 0xFFFFFFFF;
    }
    else
    {
        i = MAX(vm_start, 0x900000);
        end_addr = VIRTUAL_ADDR - PAGE_SIZE;
    }


    /* Find free page in the pool */
    for (; i < end_addr; i += PAGE_SIZE)
    {
        page = &pages_pool[i / PAGE_SIZE];
        if (!(page->flags & VMM_PAGE_AVAILABLE) || page->flags & VMM_PAGE_USED) {
            block_size = 0;
            continue;
        }

        if (block_size == 0)
            block_offset = i / PAGE_SIZE;

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

    return (void*)(block_offset * PAGE_SIZE);
}

int vmm_free_pages(void *base, size_t npages)
{
    size_t i;
    if (base == NULL)
        return 1;

    for (i = 0; i < npages; i++) {
        pages_pool[((uintptr_t)base / PAGE_SIZE) + i].flags &= ~VMM_PAGE_USED;
    }

    return 0;
}
