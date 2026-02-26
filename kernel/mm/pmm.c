#include <kernel/inari.h>
#include <kernel/mm/pmm.h>
#include <kernel/errno.h>
#include <kernel/proc/elf.h>

#include <misc/string.h>

/* TODO: The pmm should be bootloader-agnostic */
#include <multiboot/multiboot.h>

/* TODO: That's not ideal. It'd be better if we allocated the pool dynamically. */
static struct pmm_page pages_pool[PMM_POOL_SIZE];
static size_t available_pages = 0, pages_used = 0, first_good_page = 0;

extern char kern_virt_end;

static int pmm_check_overlap(uintptr_t addr, size_t len)
{
    uintptr_t kernel_start = 0;
    uintptr_t kernel_end   = (uintptr_t)&kern_virt_end - VIRTUAL_ADDR;

    if (!(addr + len <= kernel_start || addr >= kernel_end))
        return -1;
    return 0;
}

int pmm_init(void)
{
    bootinfo_t info = get_boot_info();
    multiboot_info_t *multiboot = (multiboot_info_t*)info.bootloader_info;
    multiboot_module_t *module;

    /* TODO: The memory map should be bootloader-agnostic */
    size_t i, j, len, mmap_length = multiboot->mmap_length / sizeof(struct multiboot_mmap_entry);
    uintptr_t addr;
    struct multiboot_mmap_entry *entry;
    for (i = 0; i < mmap_length; i++)
    {
        /* Parse the memory map entry from the list provided by multiboot */
        entry = &((struct multiboot_mmap_entry *)multiboot->mmap_addr)[i];
        addr = ALIGN(entry->addr, PAGE_SIZE);
        len = entry->len;

        /* Populate the pool only with those regions that are available. */
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            for (j = addr; j < addr + len; j += PAGE_SIZE)
            {
                /* Ensure that this memory region doesn't overlap important memory regions */
                if (pmm_check_overlap(j, PAGE_SIZE) != 0)
                {
                    pages_pool[j >> 12].flags = PMM_PAGE_DISABLED;
                    continue;
                }

                /* Check that we're not over the max pool size */
                if (j >> 12 >= PMM_POOL_SIZE)
                {
                    kprintf("pmm: warning - over the pool size; %ld > %ld", j >> 12, PMM_POOL_SIZE);
                    goto end;
                }

                if (!(pages_pool[j >> 12].flags & PMM_PAGE_DISABLED))
                {
                    pages_pool[j >> 12].flags = PMM_PAGE_AVAILABLE;
                    available_pages++;

                    if (first_good_page == 0 || first_good_page > j >> 12)
                        first_good_page = j >> 12;
                }
            }
        }
        else
        {
            pmm_reserve_memory((struct reserved_memory){
                .start = addr,
                .end = addr + len
            });
        }

    }

    if (multiboot->flags & MULTIBOOT_INFO_MODS)
    {
        for (i = 0; i < multiboot->mods_count; i++)
        {
            module = &((multiboot_module_t *)multiboot->mods_addr)[i];
            pmm_reserve_memory((struct reserved_memory){
                .start = module->mod_start,
                .end = module->mod_end
            });
        }
    }

    if (multiboot->flags & MULTIBOOT_INFO_ELF_SHDR) 
    {
        struct elf32_shdr* shdr = (struct elf32_shdr*)multiboot->u.elf_sec.addr;
        
        uint32_t shdr_size = multiboot->u.elf_sec.num * multiboot->u.elf_sec.size;
        pmm_reserve_memory((struct reserved_memory){
            .start = (uintptr_t)shdr,
            .end = (uintptr_t)shdr + shdr_size
        });

        for (uint32_t i = 0; i < multiboot->u.elf_sec.num; i++) 
        {
            if (shdr[i].sh_size > 0 && shdr[i].sh_addr > 0) 
            {
                pmm_reserve_memory((struct reserved_memory){
                    .start = (uintptr_t)shdr[i].sh_addr,
                    .end = (uintptr_t)shdr[i].sh_addr + shdr[i].sh_size
                });
            }
        }
    }

end:
    pmm_reserve_memory((struct reserved_memory){
        .start = 0,
        .end = (uintptr_t)&kern_virt_end - VIRTUAL_ADDR + PAGE_SIZE
    });

    kprintf("pmm: initialized, %lu pages available, %lukb.", available_pages, available_pages * (PAGE_SIZE / 1024));
    return 0;
}

void pmm_reserve_memory(struct reserved_memory region)
{
    if (region.end < region.start)
        return;

    kprintf("pmm: reserving region [0x%08x...0x%08x]", region.start, region.end);

    size_t i;
    for (i = region.start; i < region.end; i += PAGE_SIZE)
    {
        if (pages_pool[i >> 12].flags & PMM_PAGE_AVAILABLE)
        {
            available_pages--;
        }

        pages_pool[i >> 12].flags = PMM_PAGE_DISABLED;
    }
}

void *pmm_alloc_pages(size_t npages)
{
    struct pmm_page *page;
    size_t i, block_offset = 0, block_size = 0;

    if (npages == 0)
        return NULL;

    /* Check if we have available memory */
    if (pages_used + npages > available_pages)
    {
        panic("pmm: OOM during page allocation.");
        return NULL;
    }

    /* Find free page in the pool */
    for (i = first_good_page; i < PMM_POOL_SIZE; i++)
    {
        page = &pages_pool[i];
        if (!(page->flags & PMM_PAGE_AVAILABLE)) {
            block_size = 0;
            continue;
        }

        if (block_size == 0)
            block_offset = i;

        block_size++;
        if (block_size >= npages)
            break;
    }

    if (block_size < npages) {
        panic("pmm: no available contiguous blocks were found (npages = %d).", npages);
        return NULL;
    }

    /* Flag all blocks as used */
    for (i = 0; i < block_size; i++) {
        pages_pool[block_offset + i].flags |= PMM_PAGE_USED;
        pages_pool[block_offset + i].flags &= ~PMM_PAGE_AVAILABLE;
    }

    pages_used += npages;
    return (void*)(block_offset * PAGE_SIZE);
}

int pmm_free_pages(void *base, size_t npages)
{
    size_t i;
    if (base == NULL)
        return -EINVAL;

    for (i = 0; i < npages; i++) {
        if (pages_pool[((uintptr_t)base >> 12) + i].flags & PMM_PAGE_USED)
        {
            pages_pool[((uintptr_t)base >> 12) + i].flags |= PMM_PAGE_AVAILABLE;
            pages_pool[((uintptr_t)base >> 12) + i].flags &= ~PMM_PAGE_USED;
            pages_used--;
        }
    }

    return 0;
}

size_t pmm_usage()
{
    return pages_used;
}

size_t pmm_total()
{
    return available_pages;
}