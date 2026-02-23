#include <kernel/inari.h>
#include <kernel/printk.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/proc/pe.h>
#include <kernel/errno.h>

#include <arch/paging.h>
#include <misc/string.h>

extern char kern_virt_start;
extern char kern_virt_end;

int pe_load(pagedir_t *proc_pagedir, void **entrypoint, uint8_t *buf, size_t sz)
{
    int res = 0;
    void *pbase;
    size_t i, off;
    struct pe_header *header = (struct pe_header*)&buf[128];
    struct pe32_header *header32 = (struct pe32_header*)&buf[128 + sizeof(struct pe_header)];
    struct pe_image_section *section;
    struct pe_symbol *symbol;
    
    if (header->magic != PE_MAGIC) return -ENOEXEC;
    if (header32->magic != PE32_MAGIC) return -ENOEXEC;

    /* Load all sections */
    for (i = 0; i < header->sections_number; i++)
    {
        off = i * 40 + 128 + header->optional_header_sz + sizeof(struct pe_header);
        section = (struct pe_image_section*)&buf[off];

        if (vmm_check_flag(header32->image_base + section->vbase, header32->image_base + section->vbase + section->virt_sz, VMM_PAGE_RESERVED))
        {
            res = -EFAULT;
            goto end;
        }

        pbase = pmm_alloc_pages((section->virt_sz >> 12) + 1);
        vmm_disable_region((struct reserved_memory){
            .start = header32->image_base + section->vbase,
            .end = header32->image_base + section->vbase + section->virt_sz });
        if (!pbase || !arch_map_page(proc_pagedir, (void*)(header32->image_base + section->vbase), pbase, section->virt_sz, PAGE_RW | PAGE_PRESENT))
        {
            res = -ENOMEM;
            goto end;
        }

        memcpy((void*)header32->image_base + section->vbase, (void*)&buf[section->pbase], section->virt_sz);
    }

    *entrypoint = (void*)(header32->image_base + header32->address_of_entry_point);

end:
    return res;
}
