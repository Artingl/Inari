#include <kernel/kernel.h>
#include <kernel/include/C/typedefs.h>
#include <kernel/lock/spinlock.h>

#include <drivers/memory/memory.h>
#include <drivers/memory/vmm.h>
#include <drivers/memory/pmm.h>

#include <drivers/cpu/interrupts/exceptions/exceptions.h>
#include <drivers/cpu/interrupts/interrupts.h>

#include <liballoc/liballoc.h>

struct page_directory *current_directory;
struct page_directory *kernel_directory;

spinlock_t vmm_spinlock;

size_t pages_usage = 0;

int page_fault_handler(struct cpu_core *core, struct regs32 *r)
{
    spinlock_create(&vmm_spinlock);

    // extract virtual address where page fault occurred
    uintptr_t virtual_address;
    __asm__ volatile("mov %%cr2, %0"
                     : "=r"(virtual_address));

    printk(KERN_ERR "PAGE FAULT [virt=%p]!", (unsigned long)virtual_address);

    if (r->err_code & (1 << 0))
        printk(KERN_ERR "\tPage-protection violation.");
    if (r->err_code & (1 << 1))
        printk(KERN_ERR "\tWrite process.");
    if (!(r->err_code & (1 << 1)))
        printk(KERN_ERR "\tRead process.");
    if (r->err_code & (1 << 2))
        printk(KERN_ERR "\tInside userland (CPL == 3).");
    if (!(r->err_code & (1 << 2)))
        printk(KERN_ERR "\tInside kernel (CPL == 0).");
    if (r->err_code & (1 << 5))
        printk(KERN_ERR "\tProtection-key violation.");
    if (r->err_code & (1 << 6))
        printk(KERN_ERR "\tShadow stack access.");

    // panic("...");
}

void *__vmm_alloc_page_table_memory()
{
    return (void *)pmm_alloc_frames(1);
}

void vmm_init()
{
    size_t pdindex, ptindex;
    struct page_directory *core_directory = kernel_configuration()->core_directory;

    current_directory = core_directory;
    kernel_directory = core_directory;

    printk(KERN_DEBUG "Core directory phys: %p", core_directory);

    // identify first 4MB of phys memory
    vmm_identity(current_directory, 0x00, 0x400000, KERN_PAGE_RW);

    // count used pages
    for (pdindex = 0; pdindex < 1024; pdindex++)
    {
        struct page_table *table = vmm_get_table(current_directory, pdindex, false);

        if (table == NULL || (current_directory->tablesPhys[pdindex] & KERN_TABLE_PRESENT) != KERN_TABLE_PRESENT)
            continue;

        for (ptindex = 0; ptindex < 1024; ptindex++)
        {
            if (table->pages[ptindex] & KERN_PAGE_PRESENT)
                pages_usage++;
        }
    }
}

struct page_directory *vmm_current_directory()
{
    return current_directory;
}

uintptr_t vmm_get_phys(
    struct page_directory *directory,
    void *virtual)
{
    unsigned long pdindex = (unsigned long)virtual >> 22;
    unsigned long ptindex = (unsigned long)virtual >> 12 & 0x03FF;

    struct page_table *table = vmm_get_table(directory, pdindex, false);

    if (table == NULL || (directory->tablesPhys[pdindex] & KERN_TABLE_PRESENT) != KERN_TABLE_PRESENT)
        return NULL;

    return (uintptr_t)(table->pages[ptindex] & ~0xFFF);
}

void kident(void *addr, size_t length, uint32_t flags)
{
    if (!current_directory)
    {
        // We does not have any directory set for some reason...
        panic("VMM: not current directory set (is paging enabled?).");
    }

    vmm_identity(current_directory, addr, length, flags);
}

void kunident(void *addr, size_t length)
{
    if (!current_directory)
    {
        // We does not have any directory set for some reason...
        panic("VMM: not current directory set (is paging enabled?).");
    }

    vmm_unident(current_directory, addr, length);
}

size_t vmm_allocated_pages()
{
    return pages_usage;
}

int kmmap(
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags)
{
    if (!current_directory)
    {
        // We does not have any directory set for some reason...
        panic("VMM: not current directory set (is paging enabled?).");
    }

    return vmm_kmmap(current_directory, virtual, real, length, flags);
}

int vmm_kmmap(
    struct page_directory *directory,
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags)
{
    void *initial_addr = virtual;

    for (; (uintptr_t) virtual < (uintptr_t)(initial_addr + length); virtual += PAGE_SIZE)
    {
        unsigned long pdindex = (unsigned long)virtual >> 22;
        unsigned long ptindex = (unsigned long)virtual >> 12 & 0x03FF;

        struct page_table *table = vmm_get_table(directory, pdindex, true);

        table->pages[ptindex] = ((unsigned long)real) | (flags & 0xFFF) | KERN_PAGE_PRESENT;
        real += PAGE_SIZE;
    }

    return 0;
}

void vmm_identity(
    struct page_directory *directory,
    void *addr,
    size_t length,
    uint32_t flags)
{
    void *initial_addr = addr;

    for (; (uintptr_t)addr < (uintptr_t)(initial_addr + length); addr += PAGE_SIZE)
    {
        unsigned long pdindex = (unsigned long)addr >> 22;
        unsigned long ptindex = (unsigned long)addr >> 12 & 0x03FF;

        struct page_table *table = vmm_get_table(directory, pdindex, true);

        table->pages[ptindex] = ((unsigned long)addr) | (flags & 0xFFF) | KERN_PAGE_PRESENT;
    }
}

void vmm_unident(
    struct page_directory *directory,
    void *addr,
    size_t length)
{
    void *initial_addr = addr;

    for (; (uintptr_t)addr < (uintptr_t)(initial_addr + length); addr += PAGE_SIZE)
    {
        unsigned long pdindex = (unsigned long)addr >> 22;
        unsigned long ptindex = (unsigned long)addr >> 12 & 0x03FF;

        struct page_table *table = vmm_get_table(directory, pdindex, false);

        if (table != NULL)
            table->pages[ptindex] = KERN_PAGE_RW;
    }
}

struct page_table *vmm_get_table(
    struct page_directory *directory,
    unsigned long table,
    bool allocate)
{
    if (!(directory->tablesPhys[table] & KERN_PAGE_PRESENT) && allocate)
    {
        // allocate new directory
        directory->tables[table] = __vmm_alloc_page_table_memory(sizeof(struct page_table));
        if (directory->tables[table] == NULL)
            panic("VMM: got invalid address (NULL) for the table while allocating it.");

        directory->tablesPhys[table] = ((uintptr_t)directory->tables[table]) | 3;
    }

    // the page table is already allocated, return it
    return directory->tables[table];
}

void vmm_switch_directory(struct page_directory *dir)
{
    current_directory = dir;
    __asm__ volatile("mov %0, %%cr3" ::"r"(&dir->tablesPhys));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}

void *vmm_alloc_page(size_t npages)
{
    if (!current_directory)
    {
        // We does not have any directory set for some reason...
        panic("VMM: not current directory set (is paging enabled?).");
    }

    size_t i, j, total_size = 0;
    int32_t pdoffset = -1, ptoffset = -1;

    // TODO: allocate non-contiguous frames in physical memory
    uintptr_t frames = pmm_alloc_frames(npages);
    uintptr_t initial_frames = frames;

    if (!frames)
    { // unable to allocate frame of memory (maybe no memory left)
        return NULL;
    }

    // find available pages to use
    for (i = 0; i < 1024; i++)
    {
        struct page_table *table = vmm_get_table(current_directory, i, true);

        for (j = 0; j < 1024; j++)
        {
            if (total_size >= npages)
            {
                goto found_pages;
            }

            if (!(table->pages[j] & KERN_PAGE_PRESENT))
            {
                if (total_size == 0)
                {
                    pdoffset = i;
                    ptoffset = j;
                }
                total_size += 1;
            }
            else
            {
                total_size = 0;
                break;
            }
        }
    }

    if (pdoffset == -1 || ptoffset == -1)
    { // unable to find space in virtual memory
        pmm_free_frames(frames, npages);
        return NULL;
    }

found_pages:
    // allocate pages to physical address
    uintptr_t virtual_address = (pdoffset << 22) | (ptoffset << 12);

    for (; (uintptr_t)frames < (uintptr_t)(initial_frames + (PAGE_SIZE * npages)); frames += PAGE_SIZE)
    {
        struct page_table *table = vmm_get_table(current_directory, pdoffset, true);

        table->pages[ptoffset++] = ((unsigned long)frames) | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT;

        if (ptoffset >= 1024)
        {
            ptoffset = 0;
            pdoffset++;
        }
    }

    pages_usage += npages;
    return (void *)virtual_address;
}

int vmm_free_pages(
    void *frame,
    size_t npages)
{
    size_t i;
    int32_t pdindex = (unsigned long)frame >> 22;
    int32_t ptindex = (unsigned long)frame >> 12 & 0x03FF;

    uintptr_t physical_addr = (void *)(vmm_get_table(current_directory, pdindex, false)->pages[ptindex] & ~0xFFF);

    for (i = 0; i < npages; i++)
    {
        struct page_table *table = vmm_get_table(current_directory, pdindex, false);

        table->pages[ptindex++] = KERN_PAGE_RW;

        if (ptindex >= 1024)
        {
            ptindex = 0;
            pdindex++;
        }
    }

    pages_usage -= npages;
    return pmm_free_frames((uintptr_t)physical_addr, npages);
}

int liballoc_lock()
{
    return spinlock_acquire(&vmm_spinlock);
}

int liballoc_unlock()
{
    return spinlock_release(&vmm_spinlock);
}

void *liballoc_alloc(int p)
{
    return vmm_alloc_page(p);
}

int liballoc_free(void *ptr, int p)
{
    return vmm_free_pages(ptr, p);
}
