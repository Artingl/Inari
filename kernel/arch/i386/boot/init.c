#include <kernel/bootstrap.h>
#include <kernel/kernel.h>
#include <kernel/include/C/typedefs.h>

#include <kernel/arch/i386/cpu/cpu.h>
#include <kernel/arch/i386/memory/memory.h>

#include <drivers/serial/serial.h>

// Page directory that will be used by the BSP to map the kernel to higher half.
// Must be page aligned and located inside the lo-kernel data section.
struct {
    uint8_t tables_space[1024 * 1024];
    uintptr_t bsp_directory[1024];
    size_t tables_space_offset;
} pg __attribute__((aligned(PAGE_SIZE), section(".__klo_data")));

extern char __kloreal_end;
extern char __kreal_start;
extern char __kvirtual_start;
extern char __kvirtual_end;

static __lo struct page_table *i386_alloc_table(uintptr_t offset)
{
    if (!(pg.bsp_directory[offset] & KERN_TABLE_PRESENT))
    {
        struct page_table *table = (struct page_table*)&pg.tables_space[pg.tables_space_offset * sizeof(struct page_table)];
        pg.tables_space_offset++;
        pg.bsp_directory[offset] = ((uintptr_t)table) | 3; // present, RW
    }
    return (struct page_table *)((pg.bsp_directory[offset] >> 2) << 2);
}

// This is used to map the kernel to the higher half to continue booting.
// Some shit is written here, we're still really early in the boot process...
static __lo void i386_higher_kernel()
{
    uintptr_t i, vstart, vend, offset, size;
    uintptr_t pd, pt;
    uint32_t cr0;
    struct page_table *table = NULL;

    // Fill the arrays with zeros just in case
    pg.tables_space_offset = 0;
    for (i = 0; i < 1024; i++)
        pg.bsp_directory[i] = 0;
    for (i = 0; i < 1024 * 1024; i++)
        pg.tables_space[i] = 0;

    vstart = (uintptr_t)&__kvirtual_start;
    vend = (uintptr_t)&__kvirtual_end;
    offset = (uintptr_t)&__kreal_start;
    
    for (i = vstart; i < vend; i+=PAGE_SIZE)
    {
        table = i386_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)offset | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT;
        offset += PAGE_SIZE;
    }

    // Map the lower part of the kernel
    for (i = 0; i < (uintptr_t)&__kloreal_end; i+=PAGE_SIZE)
    {
        table = i386_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)i | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT;
    }

    // Enable paging
    __asm__ volatile("mov %0, %%cr3" ::"r"(&pg.bsp_directory));
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}

extern void test11();

// TODO: unable to call higher-half kernel functions. some you can call, some cannot.
//       those that cannot be called are usually pretty high in the memory (check it, only checked with serial_init)

__lo void i386_entrypoint()
{
    // remap kernel to higher half
    i386_higher_kernel();

    // fix the stack
    extern char __kstack_bottom_bsp;
    __asm__ volatile("mov %0, %%esp" :: "r"(&__kstack_bottom_bsp));
    
    // initialize serial for early debugging
    serial_init(SERIAL_COM0, 9600);

    *((int*)0xb8000)=0x07690748;

    // serial_putc(SERIAL_COM0, '!');

    // ...
    return;

    // initialize memory and the cpu
    memory_init();
    cpu_bsp_init();

    // memcpy(&payload, __payload, sizeof(struct kernel_payload));
    // go to the main kernel
    extern void kmain();
}
