#include <kernel/inari.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/page.h>
#include <arch/paging.h>

#include <misc/string.h>

int page_init(void)
{
    return 0;
}

void *page_map(void *vbase, void *pbase, size_t len, uint32_t flags)
{
    struct reserved_memory reserved = (struct reserved_memory){
        .start = (uintptr_t)vbase,
        .end = (uintptr_t)vbase + len,
    };

    /* Ensure that we can disable this region for later use */
    if (vmm_disable_region(reserved))
        return (void*)NULL;

    return arch_map_page(vbase, pbase, len, flags);
}

void page_unmap(void *vbase, size_t len)
{
    struct reserved_memory reserved = (struct reserved_memory){
        .start = (uintptr_t)vbase,
        .end = (uintptr_t)vbase + len,
    };

    /* Ensure that we can enable this region for later use */
    vmm_enable_region(reserved);

    arch_unmap_page(vbase, len);
}

void *page_alloc(size_t npages, uint32_t flags)
{
    void *vbase = vmm_alloc_pages(npages);

    /* TODO: Contiguous allocation in physical memory might fail.
     *       Allow to allocate the pages in chunks
     */
    void *pbase = pmm_alloc_pages(npages);

    return arch_map_page(vbase, pbase, npages * PAGE_SIZE, flags);
}

void page_free(void *vbase, size_t npages)
{
    for (size_t i = 0; i < npages; i++)
    {
        void *pbase = arch_virt_to_phys(vbase + i * PAGE_SIZE);
        if (pbase != NULL)
            pmm_free_pages(pbase, 1);
        vmm_free_pages(vbase, 1);
        arch_unmap_page(vbase, 1);
    }
}

pagedir_t page_get_dir(void)
{
    return (pagedir_t)arch_get_pagedir();
}

void page_switch_dir(pagedir_t dir)
{
    arch_switch_pagedir((struct paging_directory*)dir);
}

pagedir_t page_alloc_dir(void)
{
    return (pagedir_t)arch_create_pagedir();
}

void page_dealloc_dir(pagedir_t dir)
{
    if (dir) arch_cleanup_pagedir((struct paging_directory*)dir);
}

uint8_t page_is_kernel_directory(void)
{
    return arch_get_pagedir() == (struct paging_directory*)get_kernel_pagedir();
}