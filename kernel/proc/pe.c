#include <kernel/inari.h>
#include <kernel/printk.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/page.h>
#include <kernel/proc/pe.h>
#include <kernel/errno.h>

#include <misc/string.h>

#include <kernel/timer.h>
void test()
{
    printk("test routine");
    // usleep(3000000);
}

int pe_load(pagedir_t vmem, void **entrypoint, uint8_t *buf, size_t sz)
{
    int res = 0;
    void *pbase;
    size_t i, off;
    struct pe_header *header = (struct pe_header*)&buf[128];
    struct pe32_header *header32 = (struct pe32_header*)&buf[128 + sizeof(struct pe_header)];
    struct pe_image_section *section;
    struct pe_symbol *symbol;

    if (header->magic != PE_MAGIC) return -EINVAL;
    if (header32->magic != PE32_MAGIC) return -EINVAL;

    /* Switch to the vmem process will use to init it */
    pagedir_t prev_dir = page_get_dir();
    page_switch_dir(vmem);

    pbase = pmm_alloc_pages((sz >> 12) + 1);
    if (!pbase || !page_map(header32->image_base, pbase, sz, PAGE_RW | PAGE_PRESENT))
    {
        res = -ENOMEM;
        goto end;
    }
    memcpy((void*)header32->image_base, (void*)&buf[0], sz);

    /* Load all sections */
    for (i = 0; i < header->sections_number; i++)
    {
        off = i * 40 + 128 + header->optional_header_sz + sizeof(struct pe_header);
        section = (struct pe_image_section*)&buf[off];

        /* TODO: do it properly, don't ignore long names */
        if (section->name[0] == '/') continue;

        pbase = pmm_alloc_pages((section->virt_sz >> 12) + 1);
        printk("pbase 0x%x 0x%x", pbase, section->vbase);
        if (!pbase || !page_map(section->vbase, pbase, section->virt_sz, PAGE_RW | PAGE_PRESENT))
        {
            res = -ENOMEM;
            goto end;
        }

        printk("copying section %s", section->name);
        memcpy((void*)section->vbase, (void*)&buf[section->pbase], section->phys_sz);

        /* Check if this section contains entrypoint */
        if (section->vbase <= header32->address_of_entry_point
            && section->vbase + section->virt_sz > header32->address_of_entry_point)
        {
            *entrypoint = header32->image_base + (header32->address_of_entry_point - section->vbase) + section->pbase;
        }
    }

end:
    page_switch_dir(prev_dir);
    return res;
}
