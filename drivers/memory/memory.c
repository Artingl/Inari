#include <kernel/kernel.h>

#include <drivers/memory/memory.h>
#include <drivers/memory/pmm.h>
#include <drivers/memory/vmm.h>
#include <drivers/cpu/cpu.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/math.h>
#include <kernel/multiboot.h>

#include <liballoc/liballoc.h>

void memory_init()
{
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

int memory_forbid_region(uintptr_t origin, size_t size)
{
    return pmm_disable_region(origin, size);
}


void kident(void *addr, size_t length, uint32_t flags)
{
    vmm_identity(vmm_current_directory(), addr, length, flags);
}

void kunident(void *addr, size_t length)
{
    vmm_unident(vmm_current_directory(), addr, length);
}

int kmmap(
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags)
{
    return vmm_kmmap(vmm_current_directory(), virtual, real, length, flags);
}

void memory_info()
{
    size_t usage = pmm_usage() * PAGE_SIZE;
    size_t total = pmm_total() * PAGE_SIZE;

    printk("Memory info:");
    printk("\tusage: %uKB (%uMB)", usage >> 10, (usage >> 10) >> 10);
    printk("\ttotal: %uKB (%uMB)", total >> 10, (total >> 10) >> 10);
    printk("\tfree: %uKB (%uMB)", (total - usage) >> 10, ((total - usage) >> 10) >> 10);
    printk("\tused pages: %u/%u", vmm_allocated_pages(), vmm_total_pages());
}
