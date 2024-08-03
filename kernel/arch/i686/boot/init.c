#ifdef CONFIG_ARCH_I686

#include <kernel/bootstrap.h>
#include <kernel/driver/serial/serial.h>
#include <kernel/kernel.h>
#include <kernel/sys/console/console.h>
#include <kernel/include/typedefs.h>

#include <kernel/arch/i686/boot/multiboot.h>
#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/memory/memory.h>
#include <kernel/arch/i686/memory/vmm.h>

struct page_directory pg __attribute__((section(".__klo_data")));

extern char __kloreal_end;
extern char __kreal_start;
extern char __kvirtual_start;
extern char __kvirtual_end;

static __lo struct page_table *i686_alloc_table(uintptr_t offset)
{
    if (!(pg.tables_phys[offset] & KERN_TABLE_PRESENT))
    {
        struct page_table *table = (struct page_table*)&pg.tables[offset];
        pg.tables_phys[offset] = ((uintptr_t)table) | 3; // present, RW
    }
    return (struct page_table *)&pg.tables[offset];
}

// This is used to map the kernel to the higher half to continue booting.
// Some shit is written here, we're still really early in the boot process...
static __lo void i686_map_higher_kernel()
{
    uintptr_t i, vstart, vend, offset, size;
    uintptr_t pd, pt;
    uint32_t cr0;
    struct page_table *table = NULL;

    // Fill the arrays with zeros just in case
    for (i = 0; i < 1024; i++)
        pg.tables_phys[i] = ((uintptr_t)&pg.tables[offset]) | KERN_TABLE_RW;

    vstart = (uintptr_t)&__kvirtual_start;
    vend = (uintptr_t)&__kvirtual_end;
    offset = (uintptr_t)&__kreal_start;

    // Map real memory to virtual
    for (i = 0; i < 0xffffffff - (vend - vstart); i+=PAGE_SIZE)
    {
        table = i686_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)i | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT | KERN_PAGE_USED;
    }
    
    for (i = vstart; i < vend; i+=PAGE_SIZE)
    {
        table = i686_alloc_table(i >> 22);
        table->pages[i >> 12 & 0x03FF] = (unsigned long)offset | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT | KERN_PAGE_USED;
        offset += PAGE_SIZE;
    }

    // Enable paging
    __asm__ volatile("mov %0, %%cr3" ::"r"(&pg.tables_phys));
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}

__lo void i686_entrypoint(uint32_t magic, multiboot_info_t *multiboot)
{
    // Remap kernel to higher half
    i686_map_higher_kernel();

    // Fix the stack
    extern char __kstack_bottom_bsp;
    __asm__ volatile("mov %0, %%esp" :: "r"(&__kstack_bottom_bsp));
    
    // Initialize console for early debugging
    console_init(KERN_SERIAL_DEBUG);

    // First kernel message, yay!
    printk("Inari(x86) entrypoint");
    printk("Kernel virtual start: 0x%x", &__kvirtual_start);
    printk("Kernel virtual end: 0x%x", &__kvirtual_end);

    // Validate the multiboot info structure
    if (magic != MULTIBOOT_LKERNOADER_MAGIC)
    {
        panic("bootloader: invalid magic 0x%x != 0x%x", magic, MULTIBOOT_LKERNOADER_MAGIC);
        return;
    }

    printk("bootloader: magic=0x%x; info=%p; name=%s", magic, multiboot, multiboot->boot_loader_name);

    // Use memory map provided by the multiboot bootloader
    struct kernel_mmap_entry *mmap_list = (struct kernel_mmap_entry *)multiboot->mmap_addr;
    size_t mmap_list_length = multiboot->mmap_length / sizeof(struct kernel_mmap_entry);

    // Initialize memory and the cpu
    memory_init(&pg, mmap_list, mmap_list_length);
    cpu_bsp_init();

    // Get away from here to the higher kernel entrypoint
    kmain((char*)multiboot->cmdline);
}

#endif
