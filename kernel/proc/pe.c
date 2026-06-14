#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/kprintf.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/proc/pe.h>

#include <arch/paging.h>
#include <misc/string.h>

extern char kern_virt_start;
extern char kern_virt_end;

int pe_load(pagedir_t *proc_pagedir, void **entrypoint, uint8_t *buf, size_t sz) {
    int res = 0;
    void *pbase;
    size_t i, off;

    uint32_t pe_offset = 124; //*(uint32_t*)&buf[0x3C];
    // if (pe_offset >= sz || pe_offset < 64) return -ENOEXEC;

    void *dest;
    size_t copy_size;
    struct pe_header *header = (struct pe_header *)&buf[pe_offset + 4];
    struct pe32_header *header32 = (struct pe32_header *)&buf[pe_offset + 4 + sizeof(struct pe_header)];
    struct pe_image_section *section;

    if (header->magic != PE_MAGIC)
        return -ENOEXEC;
    if (header32->magic != PE32_MAGIC)
        return -ENOEXEC;

    /* Load all sections */
    for (i = 0; i < header->sections_number; i++) {
        off = i * 40 + pe_offset + 4 + header->optional_header_sz + sizeof(struct pe_header);
        if (off >= sz)
            break;

        section = (struct pe_image_section *)&buf[off];

        if (!VMM_IS_RANGE_USERSPACE(header32->image_base + section->vbase,
                                    header32->image_base + section->vbase + section->virt_sz)) {
            res = -EFAULT;
            goto end;
        }

        pbase = pmm_alloc_pages((section->virt_sz >> 12) + 1);
        vmm_disable_region(proc_pagedir,
                           (struct reserved_memory){.start = header32->image_base + section->vbase,
                                                    .end = header32->image_base + section->vbase + section->virt_sz});
        if (!pbase || !arch_map_page(proc_pagedir, (void *)(header32->image_base + section->vbase), pbase,
                                     section->virt_sz, PAGE_RW | PAGE_PRESENT | PAGE_USR)) {
            res = -ENOMEM;
            goto end;
        }

        dest = (void *)(header32->image_base + section->vbase);
        copy_size = section->phys_sz;

        /* Cap the copy size so we don't read past the file buffer */
        if (section->pbase + copy_size > sz)
            copy_size = sz - section->pbase;
        if (copy_size > section->virt_sz)
            copy_size = section->virt_sz;

        if (copy_size > 0 && section->pbase > 0)
            memcpy(dest, (void *)&buf[section->pbase], copy_size);

        /* Zero out the remainder */
        if (section->virt_sz > copy_size)
            memset((uint8_t *)dest + copy_size, 0, section->virt_sz - copy_size);
    }

    *entrypoint = (void *)(header32->image_base + header32->address_of_entry_point);

end:
    return res;
}
