#include <kernel/kernel.h>
#include <kernel/include/typedefs.h>
#include <kernel/include/math.h>
#include <kernel/include/string.h>
#include <kernel/core/lock/spinlock.h>

#include <kernel/arch/i686/memory/memory.h>
#include <kernel/arch/i686/memory/vmm.h>
#include <kernel/arch/i686/memory/pmm.h>
#include <kernel/arch/i686/cpu/cpu.h>
#include <kernel/arch/i686/cpu/interrupts/exceptions/exceptions.h>
#include <kernel/arch/i686/cpu/interrupts/interrupts.h>

#include <liballoc/liballoc.h>

spinlock_t vmm_spinlock = {0};
size_t pages_usage = 0;

int page_fault_handler(struct cpu_core *core, struct regs32 *r)
{
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
    if (r->err_code & (1 << 5))
        printk(KERN_ERR "\tProtection-key violation.");
    if (r->err_code & (1 << 6))
        printk(KERN_ERR "\tShadow stack access.");

    if (!(r->err_code & (1 << 2)))
        panic("vmm: page fault inside kernel");
    else
        panic("vmm: TODO: recover from page fault");
}

struct page_directory *kernel_directory;

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

        directory->tables_phys[pdindex] |= KERN_TABLE_PRESENT;
        struct page_table *table = vmm_get_table(directory, pdindex);

        table->pages[ptindex] = ((unsigned long)real) | (flags & 0xFFF) | KERN_PAGE_PRESENT | KERN_PAGE_USED;
        vmm_page_invalidate((uintptr_t)virtual);
        real += PAGE_SIZE;
    }

    return 0;
}

int vmm_unident(
    struct page_directory *directory,
    void *addr,
    size_t length)
{
    void *initial_addr = addr;

    for (; (uintptr_t)addr < (uintptr_t)(initial_addr + length); addr += PAGE_SIZE)
    {
        unsigned long pdindex = (unsigned long)addr >> 22;
        unsigned long ptindex = (unsigned long)addr >> 12 & 0x03FF;

        struct page_table *table = vmm_get_table(directory, pdindex);
        table->pages[ptindex] &= ~(KERN_PAGE_USED);
        vmm_page_invalidate((uintptr_t)addr);
    }

    return 0;
}

int vmm_init(struct page_directory *bsp_directory)
{
    size_t pdindex, ptindex;
    spinlock_init(&vmm_spinlock);

    // copy the bsp directory
    kernel_directory = vmm_fork_directory(bsp_directory);

    // count used pages
    for (pdindex = 0; pdindex < 1024; pdindex++)
    {
        struct page_table *table = vmm_get_table(kernel_directory, pdindex);

        if (table == NULL || (kernel_directory->tables_phys[pdindex] & KERN_TABLE_PRESENT) != KERN_TABLE_PRESENT)
            continue;

        for (ptindex = 0; ptindex < 1024; ptindex++)
        {
            if (table->pages[ptindex] & KERN_PAGE_USED)
                pages_usage++;
        }
    }

    printk("vmm: bsp_dir=0x%x; usage=%lu", bsp_directory, pages_usage);
    vmm_switch_directory(kernel_directory);

    return 0;
}

struct page_directory *vmm_fork_directory(struct page_directory *target)
{
    size_t pdindex, ptindex, i = 0;

    // Allocate new directory in the VM
    struct page_directory *fork = (struct page_directory*)pmm_alloc_frames(PD_SIZE);

    // Copy tables from the current directory
    for (pdindex = 0; pdindex < 1024; pdindex++)
    {
        fork->tables_phys[pdindex] = ((uintptr_t)&fork->tables[pdindex]) | 3;

        struct page_table *fork_table = vmm_get_table(fork, pdindex);
        struct page_table *table = vmm_get_table(target, pdindex);

        for (ptindex = 0; ptindex < 1024; ptindex++)
        {
            fork_table->pages[ptindex] = table->pages[ptindex];
        }
    }

    return fork;
}

void vmm_deallocate_directory(struct page_directory *pd)
{
    if (pd == NULL)
        return;
    pmm_free_frames((uintptr_t)pd, PD_SIZE);
}

struct page_directory *vmm_current_directory()
{
    // FIXME: current set directory
    return kernel_directory;
}

struct page_directory *vmm_kernel_directory()
{
    return kernel_directory;
}

uintptr_t vmm_get_phys(
    struct page_directory *directory,
    void *virtual)
{
    unsigned long pdindex = (unsigned long)(virtual) >> 22;
    unsigned long ptindex = (unsigned long)(virtual) >> 12 & 0x03FF;

    struct page_table *table = vmm_get_table(directory, pdindex);

    if (table == NULL || (directory->tables_phys[pdindex] & KERN_TABLE_PRESENT) != KERN_TABLE_PRESENT)
        return (uintptr_t)NULL;

    return (uintptr_t)(table->pages[ptindex] & ~0xFFF);
}

size_t vmm_allocated_pages()
{
    return pages_usage;
}

size_t vmm_total_pages()
{
    return pmm_total();
}

void vmm_switch_directory(struct page_directory *dir)
{
    if (!dir) {
        printk(KERN_ERR "vmm: invalid page directory; %p", (unsigned long)dir);
        return;       
    }

    __asm__ volatile("mov %0, %%cr3" ::"r"(&dir->tables_phys));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));
}

static void *vmm_alloc_page(size_t npages)
{
    if (npages <= 0) return NULL;

    struct page_directory *dir = vmm_current_directory();
    if (!dir)
    {
        panic("vmm: no page directory set");
        return NULL;
    }

    uintptr_t memory_size = PAGE_SIZE * npages, virt_offset;
    uintptr_t frame_ptr;
    size_t i, j, total_size = 0, result = 0;
    size_t pd_start = 0, pt_start = 0, pd_end = 0, pt_end = 0;

    // find available contiguous spot in virtual memory
    for (i = 0; i < 1024; i++)
    {
        struct page_table *table = vmm_get_table(dir, i);

        for (j = 0; j < 1024; j++)
        {
            if (total_size >= npages)
                goto end;

            if (table->pages[j] & KERN_PAGE_USED)
            {
                // Page is in use
                total_size = 0;
                continue;
            }
            else
            {
                // Found free page
                if (total_size == 0)
                {
                    pd_start = i;
                    pt_start = j;
                }
                total_size++;
            }
        }
    }

end:
    pd_end = i;
    pt_end = j;

    // unable to find space in virtual memory
    if (total_size < npages)
    {
        panic("vmm: no space in virtual memory OOPS; %d < %d", total_size, npages);
        return NULL;
    }
    
    // allocate the virtual memory
    i = pd_start;
    j = pt_start;
    do
    {
        dir->tables_phys[i] |= KERN_TABLE_PRESENT;
        struct page_table *table = vmm_get_table(dir, i);

        // Allocate frame in physical memory
        // TODO: allocate more than one frame at a time, this is a HUGE bottleneck
        frame_ptr = pmm_alloc_frames(1);
        if (frame_ptr == (uintptr_t)NULL) {
            panic("vmm: got NULL from pmm");
            return NULL;
        }

        if (table->pages[j] & KERN_PAGE_USED)
        {
            panic("vmm: unexpected used page; pd=%d pt=%d, dir=0x%x", i, j, &dir->tables_phys);
            return NULL;
        }

        // Assign the physical address to the page
        table->pages[j] = ((unsigned long)frame_ptr) | (KERN_PAGE_RW & 0xFFF) | KERN_PAGE_PRESENT | KERN_PAGE_USED;
        vmm_page_invalidate((uintptr_t)(i << 22) | (j << 12));

        j++;
        if (j >= 1024)
        {
            j = 0;
            i++;
        }

        if (i >= pd_end && j >= pt_end)
        {
            break;
        }
    } while(1);

    pages_usage += npages;
    return (void *)((pd_start << 22) | (pt_start << 12));
}

void vmm_page_invalidate(uintptr_t page)
{
    __asm__ volatile("invlpg (%0)" ::"r" (page) : "memory");
}

static int vmm_free_pages(
    void *frame,
    size_t npages)
{
    struct page_directory *dir = vmm_current_directory();
    if (!dir)
    {
        panic("vmm: no page directory set");
        return 1;
    }

    size_t i, result;
    int32_t pdindex = (unsigned long)frame >> 22;
    int32_t ptindex = (unsigned long)frame >> 12 & 0x03FF;
    uintptr_t physical_addr;

    for (i = 0; i < npages; i++)
    {
        struct page_table *table = vmm_get_table(dir, pdindex);

        // free frame
        physical_addr = (uintptr_t)(table->pages[ptindex] & ~0xFFF);
        if ((result = pmm_free_frames((uintptr_t)physical_addr, 1)) != 0)
        {
            panic("pmm: unexpected result; %d", result);
            return 1;
        }

        // mark the page as unused
        table->pages[ptindex++] &= ~(KERN_PAGE_USED);
        vmm_page_invalidate((uintptr_t)frame + i * PAGE_SIZE);

        if (ptindex >= 1024)
        {
            ptindex = 0;
            pdindex++;
        }
    }

    pages_usage -= npages;
    return 0;
}

int liballoc_lock()
{
    return spinlock_acquire(&vmm_spinlock);
}

int liballoc_unlock()
{
    return spinlock_release(&vmm_spinlock);
}

void* liballoc_alloc(size_t p)
{
    return vmm_alloc_page(p);
}

int liballoc_free(void* ptr, size_t p)
{
    return vmm_free_pages(ptr, p);
}
