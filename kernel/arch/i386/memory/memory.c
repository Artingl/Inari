#include <kernel/kernel.h>
#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/math.h>
#include <kernel/multiboot.h>

#include <kernel/arch/i386/memory/memory.h>
#include <kernel/arch/i386/memory/pmm.h>
#include <kernel/arch/i386/memory/vmm.h>
#include <kernel/arch/i386/cpu/cpu.h>

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
    size_t usage = (pmm_usage() * PAGE_SIZE) >> 10;
    size_t total = (pmm_total() * PAGE_SIZE) >> 10;
    size_t free = ((pmm_total() * PAGE_SIZE) - (pmm_usage() * PAGE_SIZE)) >> 10;

    printk("memory: usage=%uKB, total=%uKB, free=%uKB, upages=%u, tpages=%u",
        usage, total, free, vmm_allocated_pages(), vmm_total_pages());
}
