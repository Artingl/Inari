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

    // Ensure that we can disable this region for later use
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

    // Ensure that we can enable this region for later use
    vmm_enable_region(reserved);

    arch_unmap_page(vbase, len);
}

void *page_alloc(size_t npages, uint32_t flags)
{
    void *vbase = vmm_alloc_pages(npages);
    void *pbase = pmm_alloc_pages(npages);

    return arch_map_page(vbase, pbase, npages * PAGE_SIZE, flags);
}

void page_free(void *vbase, size_t npages)
{
    void *pbase = arch_phys_page(vbase);
    if (pbase != NULL)
        pmm_free_pages(pbase, npages);
    
    vmm_free_pages(vbase, npages);
    arch_unmap_page(vbase, npages * PAGE_SIZE);
}
