#include <kernel/kernel.h>
#include <kernel/list/dynlist.h>
#include <kernel/driver/memory/memory.h>

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
    kernel_assert(usage == liballoc_allocated(), "memory leak? check PMM/VMM");
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
#define NUM_TESTS 32
    size_t i, j, k;
    uint32_t *pointers[NUM_TESTS];
    uint32_t *data_pointer = NULL;
    uint32_t value, tmp_value, size, t = 0;

    // small memory check
    for (i = 0; i < NUM_TESTS; i++)
    {
        size = PAGE_SIZE * 128;
        data_pointer = kcalloc(sizeof(uint32_t), size);
        // printk("\tkcalloc -> 0x%x %lu %lu", data_pointer, size, i);
        pointers[i] = data_pointer;
        t += size;

        if (data_pointer == NULL)
            return 1;
        

        for (j = 0; j < size; j++)
        {
            value = 0x823 + j * j + i;
            data_pointer[j] = value;

            tmp_value = data_pointer[j];
            if (tmp_value != value)
            {
                return 2;
            }
        }
    }

    for (i = 0; i < NUM_TESTS; i++)
        kfree(pointers[i]);
#undef NUM_TESTS
    return 0;
}