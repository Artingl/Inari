#include <kernel/inari.h>
#include <kernel/console/earlycon.h>

#include <multiboot/multiboot.h>
#include <misc/string.h>

#include <arch/sys.h>
#include <arch/x86/cpu.h>
#include <arch/x86/arch.h>

#define _TABLE_PRESENT (1 << 0)
#define _TABLE_RW (1 << 1)

#define _PAGE_PRESENT (1 << 0)
#define _PAGE_RW (1 << 1)
#define _PAGE_USR (1 << 2)
#define _PAGE_DIRTY (1 << 5)

struct page_table
{
    uint32_t pages[1024];
};

extern char kern_virt_start;
extern char kern_virt_end;
extern char kern_phys_end;
extern char kern_phys_start;
extern char _arch_pagetable_area_start;
extern char _arch_stack_bsp;

_lo_data size_t table_pool_offset = 0;
_lo_data uintptr_t tables_phys[1024] = {0};

_lo_text static inline void *x86_memset(void *buf, int ch, size_t count)
{
    unsigned char *c = buf;
    while (count--) *c++ = (unsigned char)ch;
    return buf;
}

_lo_text static struct page_table *x86_alloc_table(uintptr_t offset)
{
    struct page_table *table_pool = (struct page_table*)ALIGN((uintptr_t)&_arch_pagetable_area_start, PAGE_SIZE);

    if (!(tables_phys[offset] & _TABLE_PRESENT))
    {
        struct page_table *table = (struct page_table*)ALIGN((uintptr_t)&table_pool[table_pool_offset++], PAGE_SIZE);
        x86_memset((void*)table, 0, sizeof(struct page_table));
        tables_phys[offset] = ((uintptr_t)table) | _TABLE_PRESENT | _TABLE_RW;
    }
    return (struct page_table *)(tables_phys[offset] & ~0xFFF);
}

_lo_text static void x86_map_section(
    uintptr_t v_start, uintptr_t v_end,
    uintptr_t p_start)
{
    struct page_table *table = NULL;
    uintptr_t offset = p_start, i;

    for (i = v_start; i < v_end; i+=PAGE_SIZE)
    {
        table = x86_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)offset | _PAGE_PRESENT | _PAGE_RW;
        offset += PAGE_SIZE;
    }
}

static void x86_entrypoint2(uint32_t magic, multiboot_info_t *multiboot)
{
    kearly_init((bootinfo_t){
        .bootloader_magic = magic,
        .bootloader_info = multiboot,
        .cmdline = (const char*)multiboot->cmdline,
    });
    
    x86_cpu_init();
    
    /* Jump to kernel! */
    kmain();
}

/* This is the main entrypoint in the kernel for the x86 cpus.
 * Here we setup the MMU by mapping kernel to higher half, and some other things.
*/
 _lo_text void x86_entrypoint(uint32_t magic, multiboot_info_t *multiboot)
{
    size_t i;

    /* Alloc all 1024 page tables to avoid pain */
    for (i = 0; i < 1024; i++)
        x86_alloc_table(i);

    /* Map crucial memory regions */
    x86_map_section((uintptr_t)&kern_phys_start, (uintptr_t)&kern_phys_end, (uintptr_t)&kern_phys_start);
    x86_map_section((uintptr_t)&kern_virt_start, (uintptr_t)&kern_virt_end, (uintptr_t)&kern_phys_end);

    /* Enable paging */
    uint32_t cr0;
    __asm__ volatile("mov %0, %%cr3" ::"r"(&tables_phys));
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    /* Fix the stack */
    __asm__ volatile("mov %0, %%esp" :: "r"(&_arch_stack_bsp));

    /* We can finally go further */
    x86_entrypoint2(magic, multiboot);

    halt();
}
