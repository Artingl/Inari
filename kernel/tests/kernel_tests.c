#include <kernel/kernel.h>

void kernel_make_tests()
{
    printk(KERN_INFO "Testing the kernel...");
    kernel_assert(kernel_mem_test() == 0, "memory test failed");
    printk(KERN_INFO "Kernel passed all tests!");
}

int kernel_mem_test()
{
    // small memory check
    uint32_t *data = kmalloc(512);
    for (size_t i = 0; i < 512; i++)
        data[i] = i * i;
    printk(KERN_DEBUG "MEMTEST: kmalloc: %p (phys: %p)", data, vmm_get_phys(vmm_current_directory(), data));

    for (size_t i = 0; i < 512; i++) {
        if (data[i] != i * i)
            return -1;
    }

    kfree(data);
    return 0;
}