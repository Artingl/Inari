#include <kernel/kernel.h>
#include <kernel/list/dynlist.h>

#include <kernel/arch/i386/memory/memory.h>
#include <kernel/arch/i386/memory/pmm.h>

#include <liballoc/liballoc.h>

int kernel_mem_test();
int kernel_dynlist_test();

void do_kern_tests()
{
    int result;
    size_t usage = liballoc_allocated();
    memory_info();

    printk("kernel: running self tests...");
    if ((result = kernel_mem_test()) != 0) panic("kern_tests: memory test failed; code: %d", result);
    kernel_assert(usage == liballoc_allocated(), "memory leak? check PMM/VMM");
    if ((result = kernel_dynlist_test()) != 0) panic("kern_tests: dynlist test failed; code: %d", result);
    printk("kernel: passed all tests");
}

int kernel_dynlist_test()
{
    dynlist(list);

    int rnd0 = __COUNTER__,
        rnd1 = __COUNTER__ + 16,
        rnd2 = __COUNTER__ + 32,
        rnd3 = __COUNTER__ + 48,
        rnd4 = __COUNTER__ + 64,
        rnd5 = __COUNTER__ + 80;

    dynlist_append(list, rnd0);
    dynlist_append(list, rnd1);
    dynlist_append(list, rnd2);
    dynlist_append(list, rnd3);
    dynlist_append(list, rnd4);
    dynlist_append(list, rnd5);

    if (dynlist_size(list) != 6)
        return 1;

    if (dynlist_get(list, 3, int) != rnd3) return 2;
    if (dynlist_get(list, 0, int) != rnd0) return 2;
    if (dynlist_get(list, 4, int) != rnd4) return 2;
    if (dynlist_remove(list, 3) != 0) return 3;
    if (dynlist_remove(list, 1) != 0) return 3;
    if (dynlist_get(list, 1, int) != rnd2) return 4;
    if (dynlist_get(list, 2, int) != rnd4) return 4;
    if (dynlist_size(list) != 4) return 5;
    if (dynlist_set(list, 0, rnd4) != 0) return 6;
    if (dynlist_set(list, 1, rnd2) != 0) return 6;
    if (dynlist_set(list, 2, rnd5) != 0) return 6;
    if (dynlist_pop(list, int) != rnd5) return 7;
    if (dynlist_pop(list, int) != rnd5) return 7;
    if (dynlist_pop(list, int) != rnd2) return 7;
    if (dynlist_pop(list, int) != rnd4) return 7;
    if (dynlist_size(list) != 0) return 8;
    if (dynlist_set(list, 0, 342) != 1) return 9;
    if (dynlist_set(list, 3, 342) != 1) return 9;
    if (dynlist_set(list, 1, 342) != 1) return 9;
    if (dynlist_size(list) != 0) return 10;
    if (dynlist_insert(list, 0, rnd0) != 0) return 11;
    if (dynlist_insert(list, 3, rnd1) != 3) return 11;
    if (dynlist_size(list) != 4) return 12;
    if (dynlist_insert(list, 1, rnd2) != 1) return 13;
    if (dynlist_size(list) != 5) return 14;
    if (dynlist_pop(list, int) != rnd1) return 15;
    if (dynlist_pop(list, int) != 0) return 15;
    if (dynlist_pop(list, int) != 0) return 15;
    if (dynlist_pop(list, int) != rnd2) return 15;
    if (dynlist_pop(list, int) != rnd0) return 15;
    if (dynlist_size(list) != 0) return 16;
    if (dynlist_insert(list, 0, rnd5) != 0) return 16;
    if (dynlist_insert(list, 0, rnd3) != 0) return 16;
    if (dynlist_pop(list, int) != rnd5) return 17;
    if (dynlist_pop(list, int) != rnd3) return 17;
    if (dynlist_size(list) != 0) return 18;

    return 0;
}

int kernel_mem_test()
{
    // size_t i, j, k;
    // uint32_t *pointers[90];
    // uint32_t *data_pointer;
    // uint32_t value, tmp_value, size, t = 0;

    // struct page_directory *dir = vmm_current_directory();

    // // small memory check
    // for (i = 0; i < 48; i++)
    // {
    //     size = PAGE_SIZE + i * PAGE_SIZE;
    //     data_pointer = kcalloc(sizeof(uint32_t), size);
    //     pointers[i] = data_pointer;
    //     t += size;

    //     if (data_pointer == NULL)
    //         return 1;

    //     for (j = 0; j < size; j++) {
    //         value = 0x823 + j * j + i;
    //         data_pointer[j] = value;

    //         tmp_value = data_pointer[j];
    //         if (tmp_value != value) {
    //             printk(KERN_ERR "kern_tests: real shit happened here... %d != %d", tmp_value, value);
    //             printk(KERN_ERR "kern_tests: pointers:");
    //             for (k = 0; k <= i; k++)
    //                 printk(KERN_ERR "kern_tests: \tvirt: %p, phys: %p", (unsigned long)pointers[k], (unsigned long)vmm_get_phys(dir, (void*)pointers[k]));
    //             return 2;
    //         }
    //     }
    // }

    // for (i = 0; i < 90; i++)
    //     kfree(pointers[i]);
    return 0;
}