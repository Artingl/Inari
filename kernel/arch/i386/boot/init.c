#include <kernel/bootstrap.h>
#include <kernel/kernel.h>
#include <kernel/include/C/typedefs.h>

#include <kernel/arch/i386/cpu/cpu.h>
#include <kernel/arch/i386/memory/memory.h>

#include <drivers/serial/serial.h>

// Page directory that will be used by the BSP to map the kernel to higher half.
// Must be page aligned and located inside the lo-kernel data section.
size_t tables_space_offset = 0;
uint8_t tables_space[1024 * 1024]
    __attribute__((aligned(PAGE_SIZE), section(".__klo_data")));

uintptr_t bsp_directory[1024]
    __attribute__((aligned(PAGE_SIZE), section(".__klo_data")));

extern char __kloreal_end;
extern char __kreal_start;
extern char __kvirtual_start;
extern char __kvirtual_end;

static __lo struct page_table *i386_alloc_table(uintptr_t offset)
{
    if (bsp_directory[offset] == (uintptr_t)NULL)
    {
        bsp_directory[offset] = ((uintptr_t)&tables_space[tables_space_offset]) | 3;
        tables_space_offset += sizeof(struct page_table);
    }
    return (struct page_table *)((bsp_directory[offset] >> 2) << 2);
}

// This is used to map the kernel to the higher half to continue booting.
// Some shit is written here, we're still really early in the boot process...
static __lo void i386_higher_kernel()
{
    size_t i, offset, size;
    uintptr_t pd, pt;
    uint32_t cr0;
    struct page_table *table = NULL;

    // Fill the arrays with zeros just in case
    for (i = 0; i < sizeof(bsp_directory); i++)
        *(((uint8_t*)&bsp_directory) + i) = 0;
    for (i = 0; i < sizeof(tables_space); i++)
        *(((uint8_t*)&tables_space) + i) = 0;

    offset = (size_t)&__kreal_start;
    size = (size_t)(&__kvirtual_end - &__kvirtual_start);
    for (i = 0; i < size; i+=PAGE_SIZE)
    {
        pd = (i + (uintptr_t)&__kvirtual_start) >> 22;
        pt = (i + (uintptr_t)&__kvirtual_start) >> 12 & 0x03FF;

        table = i386_alloc_table(pd);
        table->pages[pt] = ((unsigned long)offset) | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT | KERN_PAGE_USED;
        offset += PAGE_SIZE;
    }

    // Map the lower part of the kernel
    for (i = 0; i < (size_t)&__kloreal_end; i+=PAGE_SIZE)
    {
        table = i386_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)i | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT | KERN_PAGE_USED;
    }

    // Enable paging
    __asm__ volatile("mov %0, %%cr3" ::"r"(&bsp_directory));
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}

static void test11()
{
    *((uint8_t*)0xb8002)='A';
    *((uint8_t*)0xb8003)='A';
    *((uint8_t*)0xb8004)='A';
    *((uint8_t*)0xb8005)='A';
}

// TODO: unable to call higher-half kernel functions. some you can call, some cannot.
//       those that cannot be called are usually pretty high in the memory (check it, only checked with serial_init)

__lo void i386_entrypoint()
{
    // remap kernel to higher half
    i386_higher_kernel();

    // fix the stack
    extern char __kstack_bottom_bsp;
    __asm__ volatile("mov %0, %%esp" :: "r"(&__kstack_bottom_bsp));
    
    test11();
    *((uint8_t*)0xb8000)='A';

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
