#include <kernel/kernel.h>

#include <drivers/memory/pmm.h>

void kernel_make_tests()
{
    size_t usage = pmm_usage() * PAGE_SIZE;
    size_t total = pmm_total() * PAGE_SIZE;

    printk(KERN_INFO "Testing the kernel...");
    kernel_assert(kernel_mem_test() == 0, "memory test failed");
    kernel_assert(usage == pmm_usage() * PAGE_SIZE, "memory leak?");
    kernel_assert(total == pmm_total() * PAGE_SIZE, "memory leak?");
    printk(KERN_INFO "Kernel passed all tests!");
}

int kernel_mem_test()
{
    // small memory check
    for (size_t _ = 0; _ < 0x2000; _++)
    {
        uint32_t *data = kmalloc(1024);
        for (size_t i = 0; i < 1024; i++)
            data[i] = i * i + _;

        for (size_t i = 0; i < 1024; i++)
        {
            if (data[i] != i * i + _) 
                return -1;
        }

        kfree(data);
    }
    return 0;
}