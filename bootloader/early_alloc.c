#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/math.h>
#include <kernel/kernel.h>

#include <bootloader/lower.h>

extern void *lo_early_heap_top;
extern void *lo_early_heap;
extern void *lo_early_heap_end;

extern void *_kernel_phys_start;
extern void *_kernel_phys_end;

extern void *_lo_start_marker;
extern void *_lo_end_marker;


LKERN void early_alloc_setup(multiboot_info_t *multiboot)
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
    ln = ln > PAGE_SIZE * 32 ? PAGE_SIZE * 32 : ln;

    *(&lo_early_heap_top) = (void*)ptr;
    *(&lo_early_heap) = (void *)ptr + PAGE_SIZE;
    *(&lo_early_heap_end) = (void *)(ln + ptr);
}

LKERN struct early_alloc_info early_alloc_info()
{
    return (struct early_alloc_info){
        .heap_top = (uintptr_t)*(&lo_early_heap_top),
        .heap = (uintptr_t)*(&lo_early_heap),
        .heap_end = (uintptr_t)*(&lo_early_heap_end),
    };
}

LKERN void *early_alloc(size_t length)
{
    void *mem = NULL;

    do
    {
        mem = *(&lo_early_heap);
        *(&lo_early_heap) += align(length, PAGE_SIZE);

        if ((uintptr_t)mem > (uintptr_t)&_kernel_phys_end && (uintptr_t)*(&lo_early_heap) > (uintptr_t)&_kernel_phys_end)
            break;
    }
    while (true);

    return mem;
}
