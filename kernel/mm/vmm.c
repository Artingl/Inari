#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/kmalloc.h>
#include <misc/list.h>
#include <kernel/proc/sched.h>

#include <arch/paging.h>
#include <misc/string.h>

static LIST_HEAD(pagedir_pool);

static struct vmm_pool_entry kernel_pool_entry;
extern char kern_phys_end;
extern char kern_virt_end;

static struct vmm_pool_entry *get_pool_entry(pagedir_t *pagedir)
{
    struct list_head *pos;
    struct vmm_pool_entry *entry;

    if (pagedir == arch_get_kernel_pagedir()) return &kernel_pool_entry;

    list_for_each(pos, &pagedir_pool) {
        entry = list_entry(pos, struct vmm_pool_entry, list);
        if (entry->pagedir == pagedir)
            return entry;
    }

    return NULL;
}

static int mark_region(struct vmm_pool_entry *entry, uintptr_t start, uintptr_t end, uint8_t flags)
{
    uintptr_t i;
    if (start < entry->pool_offset) return -EINVAL;
    for (i = start; i < end; i += PAGE_SIZE)
    {
        if (!(entry->pages_pool[(i - entry->pool_offset) >> 12].flags & VMM_PAGE_RESERVED))
            entry->pages_pool[(i - entry->pool_offset) >> 12].flags = flags;
    }

    return 0;
}

int vmm_init(void)
{
    /* Allocate some memory for the kernel page pool */
    kernel_pool_entry.pagedir = arch_get_kernel_pagedir();
    kernel_pool_entry.pool_offset = VIRTUAL_ADDR;
    kernel_pool_entry.pages_pool = (struct vmm_page *)VMM_KERN_VBASE;

    /* Mark the whole kernel's virtual memory as available at first */
    memset((void*)kernel_pool_entry.pages_pool, VMM_PAGE_AVAILABLE, VMM_KERN_SIZE_BYTES);
    
    /* Disable memory regions in kernel memory */
    mark_region(&kernel_pool_entry, VMM_KERN_VBASE, VMM_KERN_VBASE + VMM_KERN_SIZE_BYTES, VMM_PAGE_RESERVED);
    mark_region(&kernel_pool_entry, VIRTUAL_ADDR, (uintptr_t)&kern_virt_end + PAGE_SIZE, VMM_PAGE_RESERVED);

    printk("vmm: initialized");
    return 0;
}

int vmm_disable_region(pagedir_t *target_dir, struct reserved_memory region)
{
    return mark_region(get_pool_entry(target_dir), region.start, region.end, VMM_PAGE_USED);
}

static void *alloc_range(struct vmm_pool_entry *entry, size_t npages, uintptr_t from_mem, uintptr_t to_mem, uint32_t flags)
{
    if (!entry || from_mem < entry->pool_offset) return (void*)NULL;
 
    struct vmm_page *page;
    /* TODO: try to allocate physical memory in chunks, not contiguous */
    void *pbase = pmm_alloc_pages(npages);
    size_t block_vbase = 0, block_size = 0, i;

    if (npages == 0 || !pbase)
        return NULL;

    /* Find free page in the pool */
    for (; from_mem < to_mem; from_mem += PAGE_SIZE)
    {
        page = &entry->pages_pool[(from_mem - entry->pool_offset) >> 12];
        if (!(page->flags & VMM_PAGE_AVAILABLE)) {
            block_size = 0;
            continue;
        }

        if (block_size == 0)
            block_vbase = from_mem;

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
        entry->pages_pool[((block_vbase - entry->pool_offset) >> 12) + i].flags |= VMM_PAGE_USED;
        entry->pages_pool[((block_vbase - entry->pool_offset) >> 12) + i].flags &= ~(VMM_PAGE_AVAILABLE);
    }

    /* Map all the physical memory to virtual memory and return new vbase */
    return arch_map_page(entry->pagedir, (void*)block_vbase, pbase, npages * PAGE_SIZE, flags);
}

void *vmm_alloc_user(pagedir_t *target_dir, size_t npages)
{
    struct vmm_pool_entry *entry = get_pool_entry(target_dir);
    if (!entry) return NULL;
    
    uintptr_t start_addr = (uintptr_t)&kern_phys_end + PAGE_SIZE;
    if (start_addr < 0x1000000) {
        start_addr = 0x1000000;
    }
    
    return alloc_range(
        entry,
        npages,
        start_addr,
        VIRTUAL_ADDR - PAGE_SIZE,
        PAGE_PRESENT | PAGE_RW | PAGE_USR
    );
}

void *vmm_alloc_kernel(size_t npages)
{
    return alloc_range(
        &kernel_pool_entry,
        npages,
        VIRTUAL_ADDR,
        0xFFFFFFFF,
        PAGE_PRESENT | PAGE_RW
    );
}

static int free_pagedir(struct vmm_pool_entry *entry, void *base, size_t npages)
{
    size_t i;
    if (!base || !entry)
        return -1;

    for (i = 0; i < npages; i++)
    {
        if (entry->pages_pool[(((uintptr_t)base - entry->pool_offset) >> 12) + i].flags & VMM_PAGE_USED)
        {
            void *pbase = arch_virt_to_phys(entry->pagedir, base + i * PAGE_SIZE);
            if (pbase != NULL)
                pmm_free_pages(pbase, 1);
            entry->pages_pool[(((uintptr_t)base - entry->pool_offset) >> 12) + i].flags &= ~(VMM_PAGE_USED);
            entry->pages_pool[(((uintptr_t)base - entry->pool_offset) >> 12) + i].flags |= VMM_PAGE_AVAILABLE;
        }
    }

    arch_unmap_page(entry->pagedir, base, npages);
    return 0;
}

int vmm_free_pages(pagedir_t *target_dir, void *base, size_t npages)
{
    struct vmm_pool_entry *entry = get_pool_entry(target_dir);
    return free_pagedir(entry, base, npages);
}

void vmm_init_directory(pagedir_t *pagedir)
{
    if (!pagedir)
        return;
    struct vmm_pool_entry *entry = (struct vmm_pool_entry *)kmalloc(sizeof(struct vmm_pool_entry));
    if (!entry) panic("vmm: OOM during directory init.");

    entry->pages_pool = (struct vmm_page *)kmalloc(VMM_USR_SIZE_BYTES);
    if (!entry->pages_pool) panic("vmm: OOM during directory init.");

    entry->pool_offset = (uintptr_t)(&kern_phys_end + PAGE_SIZE);
    entry->pagedir = pagedir;

    /* Mark the whole virtual memory as available at first */
    memset((void*)entry->pages_pool, VMM_PAGE_AVAILABLE, VMM_USR_SIZE_BYTES);

    /* Disable lower physical memory */
    uintptr_t reserve_end = (uintptr_t)&kern_phys_end + PAGE_SIZE;
    if (reserve_end < 0x1000000) {
        reserve_end = 0x1000000;
    }
    mark_region(entry, 0, reserve_end, VMM_PAGE_RESERVED);

    INIT_LIST_HEAD(&entry->list);
    list_add(&entry->list, &pagedir_pool);
}

void vmm_cleanup_directory(pagedir_t *pagedir)
{
    if (!pagedir)
        return;

    size_t i;
    uintptr_t vaddr;
    void *pbase;
    struct list_head *pos, *n;
    struct vmm_pool_entry *entry;

    list_for_each_safe(pos, n, &pagedir_pool) {
        entry = list_entry(pos, struct vmm_pool_entry, list);
        if (entry->pagedir == pagedir) {
            list_del(&entry->list);

            /* Clean up all physical memory used by thi directory */
            for (i = 0; i < VMM_USR_SIZE_BYTES / sizeof(struct vmm_page); i++)
            {
                if (entry->pages_pool[i].flags & VMM_PAGE_USED)
                {
                    vaddr = entry->pool_offset + (i * PAGE_SIZE);
                    pbase = arch_virt_to_phys(pagedir, (void*)vaddr);
                    if (pbase)
                        pmm_free_pages(pbase, 1);
                }
            }

            kfree(entry->pages_pool);
            kfree(entry);
            return;
        }
    }
}
