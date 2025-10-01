#include <kernel/inari.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/pmm.h>

#include <arch/paging.h>
#include <misc/string.h>

static struct vmm_frame *frames_pool;
static uintptr_t vm_start, vm_end;

static inline void __disable_region(uintptr_t base, size_t len)
{
    size_t i;
    for (i = base; i < base + len; i += KERN_PAGE_SIZE)
    {
        frames_pool[i / KERN_PAGE_SIZE].flags = VMM_FRAME_DISABLED;
    }
}

int vmm_init(void)
{
    size_t i;

    // Allocate some memory for the VMM frames pool
    frames_pool = arch_map_page(
        (void*)VMM_VBASE,
        (void*)pmm_alloc_frames(VMM_SIZE_FRAMES),
        VMM_SIZE_BYTES,
        0);
    
    // Disable memory regions that are occupied by mapped kernel, or by identity mapping (below kern_phys_end)
    extern char kern_phys_end;

    __disable_region(0, (size_t)&kern_phys_end);
    __disable_region(VMM_VBASE, 0xffffffff);

    vm_start = (uintptr_t)&kern_phys_end + KERN_PAGE_SIZE;
    vm_end = VMM_VBASE - KERN_PAGE_SIZE;

    for (i = vm_start; i < vm_end; i += KERN_PAGE_SIZE)
        frames_pool[i / KERN_PAGE_SIZE].flags = VMM_FRAME_AVAILABLE;

    printk("vmm: initialized, base 0x%08x, boundaries 0x%08x...0x%08x", frames_pool, vm_start, vm_end);
    return 0;
}

uintptr_t vmm_alloc_frames(size_t nframes)
{
    struct vmm_frame *frame;
    size_t block_offset = 0, block_size = 0;
    uintptr_t i;

    if (nframes == 0)
        return (uintptr_t)NULL;

    // Find free frame in the pool
    for (i = vm_start; i < vm_end; i += KERN_PAGE_SIZE)
    {
        frame = &frames_pool[i / KERN_PAGE_SIZE];
        if (!(frame->flags & VMM_FRAME_AVAILABLE) || frame->flags & VMM_FRAME_USED) {
            block_size = 0;
            continue;
        }

        if (block_size == 0)
            block_offset = i / KERN_PAGE_SIZE;

        block_size++;
        if (block_size >= nframes)
            break;
    }

    if (block_size < nframes) {
        panic("vmm: no available contiguous blocks were found (nframes = %d).", nframes);
        return (uintptr_t)NULL;
    }

    // Flag all blocks as used
    for (i = 0; i < block_size; i++) {
        frames_pool[block_offset + i].flags |= VMM_FRAME_USED;
    }

    return block_offset * KERN_PAGE_SIZE;
}

int vmm_free_frames(uintptr_t frames, size_t nframes)
{
    size_t i;
    if (frames == (uintptr_t)NULL)
        return 1;

    for (i = 0; i < nframes; i++) {
        frames_pool[(frames / KERN_PAGE_SIZE) + i].flags &= ~VMM_FRAME_USED;
    }

    return 0;
}
