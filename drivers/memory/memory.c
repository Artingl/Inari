#include <kernel/kernel.h>

#include <drivers/memory/memory.h>
#include <drivers/memory/pmm.h>
#include <drivers/memory/vmm.h>
#include <drivers/cpu/interrupts/idt/idt.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/multiboot.h>

#include <liballoc/liballoc.h>

void memory_init()
{
    // initialize IDT structures, because we'll need to subscribe for
    // page fault interrupt later, and without it we might get some unexpected errors.
    cpu_idt_init_memory();

    // initialize Physical Memory Manager and Virtual Memory Manager
    pmm_init();
    vmm_init();
}

void *kmalloc(size_t length)
{
    return kmalloc_real(length);
}

void kfree(void *ptr)
{
    kfree_real(ptr);
}


void *krealloc(void *ptr, size_t size)
{
    return krealloc_real(ptr, size);
}


void *kcalloc(size_t n, size_t size)
{
    return kcalloc_real(n, size);
}

void memory_info()
{
    size_t usage = pmm_usage() * PAGE_SIZE;
    size_t total = pmm_total() * PAGE_SIZE;

    printk(KERN_INFO "Memory info:");
    printk(KERN_INFO "\tusage: %dKB (%dMB)", usage / 1024, usage / 1024 / 1024);
    printk(KERN_INFO "\ttotal: %dKB (%dMB)", total / 1024, total / 1024 / 1024);
    printk(KERN_INFO "\tfree: %dKB (%dMB)", (total - usage) / 1024, (total - usage) / 1024 / 1024);
    printk(KERN_INFO "\tused pages: %d", vmm_allocated_pages());
}
