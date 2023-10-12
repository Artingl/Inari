#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/math.h>

#include <bootloader/lower.h>

extern void *_lower_early_heap_top;
extern void *_lower_early_heap;
extern void *_lower_early_heap_end;

extern void *_kernel_phys_start;
extern void *_kernel_phys_end;

extern void *_bootloader_start;
extern void *_bootloader_end;


BOOTL void early_alloc_setup(multiboot_info_t *multiboot)
{
    multiboot_memory_map_t *entry;

    uintptr_t ln = 0, ptr = (uintptr_t)NULL;

    // search for available memory
    for (
        entry = (multiboot_memory_map_t *)multiboot->mmap_addr;
        ((uintptr_t)entry) < multiboot->mmap_addr + multiboot->mmap_length;
        entry++)
    {
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            if (ln < entry->len)
            {
                ln = entry->len;
                ptr = entry->addr;
            }
        }
    }

    if (!ptr)
    {
        bl_panic(MESSAGES_POOL[MSG_NO_MMAPS]);
    }

    // align the ptr by the page size
    ptr = align(ptr, PAGE_SIZE);

    *(&_lower_early_heap_top) = (void*)ptr;
    *(&_lower_early_heap) = (void *)ptr + PAGE_SIZE;
    *(&_lower_early_heap_end) = (void *)(ln + ptr);
}

BOOTL struct early_alloc_info early_alloc_info()
{
    return (struct early_alloc_info){
        .heap_top = *(&_lower_early_heap_top),
        .heap = *(&_lower_early_heap),
        .heap_end = *(&_lower_early_heap_end),
    };
}

BOOTL void *early_alloc(size_t length)
{
    void *mem = NULL;

    do
    {
        mem = *(&_lower_early_heap);
        *(&_lower_early_heap) += align(length, PAGE_SIZE);

        if (mem > &_kernel_phys_end && *(&_lower_early_heap) > &_kernel_phys_end)
            break;
    }
    while (true);

    return mem;
}
