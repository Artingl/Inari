#include <kernel/kernel.h>

#include <drivers/memory/memory.h>
#include <drivers/memory/pmm.h>

#include <liballoc/liballoc.h>

int kernel_mem_test();

void kernel_make_tests()
{
    int result;
    size_t usage = liballoc_allocated();
    memory_info();

    printk("kernel: running self tests...");
    if ((result = kernel_mem_test()) != 0) panic("kern_tests: memory test failed; code: %d", result);
    kernel_assert(usage == liballoc_allocated(), "memory leak? check PMM/VMM");
    printk("kernel: passed all tests");
}

int kernel_mem_test()
{
    size_t i, j, k;
    uint32_t *pointers[90];
    uint32_t *data_pointer;
    uint32_t value, tmp_value, size, t = 0;

    struct page_directory *dir = vmm_current_directory();

    // small memory check
    for (i = 0; i < 32; i++)
    {
        size = PAGE_SIZE + i * PAGE_SIZE;
        data_pointer = kcalloc(sizeof(uint32_t), size);
        pointers[i] = data_pointer;
        t += size;

        if (data_pointer == NULL)
            return 1;

        for (j = 0; j < size; j++) {
            value = 0x823 + j * j + i;
            data_pointer[j] = value;

            tmp_value = data_pointer[j];
            if (tmp_value != value) {
                printk(KERN_ERR "kern_tests: real shit happened here... %d != %d", tmp_value, value);
                printk(KERN_ERR "kern_tests: pointers:");
                for (k = 0; k <= i; k++)
                    printk(KERN_ERR "kern_tests: \tvirt: %p, phys: %p", (unsigned long)pointers[k], (unsigned long)vmm_get_phys(dir, (void*)pointers[k]));
                return 2;
            }
        }
    }

    for (i = 0; i < 90; i++)
        kfree(pointers[i]);
    return 0;
}