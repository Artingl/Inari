#include <kernel/kernel.h>
#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

#include <kernel/arch/i386/memory/pmm.h>
#include <kernel/arch/i386/cpu/interrupts/apic/io_apic.h>
#include <kernel/arch/i386/cpu/interrupts/apic/local_apic.h>

struct page_frame frames_pool[PMM_POOL_SIZE];
size_t frames_pool_size = 0;
size_t frames_used = 0;
int64_t frames_first_good_block = -1;

#define CHECK_OVERLAP(x0, x1, v) ((uintptr_t)(x0) <= (uintptr_t)(v) && (uintptr_t)(x1) >= (uintptr_t)(v))

int pmm_check_overlap(uintptr_t addr);

int pmm_init()
{
    size_t i, j, overlaps = 0, len;
    uintptr_t addr;
    struct kernel_payload const *payload = kernel_configuration();
    struct kernel_mmap_entry *mmap;

    printk("pmm: memory maps base = %p, len = %d", (unsigned int)payload->mmap, payload->mmap_length);
    printk("pmm: page_frame_t base = %p, size = %x", (unsigned int)&frames_pool[0], (unsigned int)sizeof(frames_pool));
    memset((void*)&frames_pool[0], 0, sizeof(frames_pool));

    for (i = 0; i < payload->mmap_length; i++)
    {
        mmap = &payload->mmap[i];
        addr = align(mmap->addr, PAGE_SIZE);
        len = mmap->len;

        if (mmap->type == KERN_MMAP_AVAILABLE && addr >= 0x100000)
        {
            // Use the memory region we found
            printk("pmm: using memory region from %p to %p", (unsigned long)addr, (unsigned long)(addr + len));

            // allocate frames for found available memory
            for (j = addr; j < addr + len; j += PAGE_SIZE)
            {
                // check for overlaps
                if (pmm_check_overlap(j) != 0)
                {
                    overlaps++;
                    continue;
                }

                if (!(frames_pool[j >> 12].flags & KERN_MMAP_AVAILABLE))
                {
                    frames_pool[j >> 12].base = j;
                    frames_pool[j >> 12].flags = PMM_FRAME_AVAILABLE;
                    frames_pool_size++;

                    if (frames_first_good_block == -1)
                        frames_first_good_block = j >> 12;
                }
            }
        }
        else {
            // Ignore any other memory regions
            pmm_disable_region(addr, len);
        }
    }
    
    printk("pmm: total frames %d (%dKB), %d overlaps", frames_pool_size, frames_pool_size * PAGE_SIZE / 1024, overlaps);
    return 0;
}

extern char __kreal_start;
extern char __kreal_end;

int pmm_check_overlap(uintptr_t addr)
{
    uintptr_t kernel_start = (uintptr_t)&__kreal_start;
    uintptr_t kernel_end = (uintptr_t)&__kreal_end;
    uintptr_t kernel_length = kernel_end - kernel_start;


    if (CHECK_OVERLAP(0x00100000, 0x01000000, addr)) return 1;
    if (CHECK_OVERLAP(0x00F00000, 0x00FFFFFF, addr)) return 1;
    if (CHECK_OVERLAP(0xC0000000, 0xFFFFFFFF, addr)) return 1;

    // make sure we don't overwrite the bootloader's memory
    // TODO: We should not directly get the info from early allocator.
    //       Instead the bootloader must mark the region of the early allocator as not available.
    // struct early_alloc_info early_memory = early_alloc_info();
    // if (CHECK_OVERLAP(0, early_memory.heap_end, addr)) return 1;

    // also we need to be sure not to overwrite the kernel's memory
    if (CHECK_OVERLAP(0, kernel_end, addr)) return 1;

    return 0;
}

int pmm_disable_region(uintptr_t origin, size_t size)
{
    printk("pmm: ignoring memory region from %p to %p", (unsigned long)origin, (unsigned long)(origin + size));

    size_t i;
    size_t nframes = size / PAGE_SIZE;
    nframes = nframes == 0 ? 1 : nframes;
    for (i = 0; i < nframes; i++) {
        if (frames_pool[(origin >> 12) + i].flags & KERN_MMAP_AVAILABLE)
        {
            frames_pool_size--;
        }

        frames_pool[(origin >> 12) + i].base = (uintptr_t)NULL;
        frames_pool[(origin >> 12) + i].flags = PMM_FRAME_NOT_AVAILABLE;
    }
    return 0;
}

uintptr_t pmm_alloc_frames(size_t nframes)
{
    struct page_frame *frame;
    size_t i, block_offset = 0, block_size = 0;

    // Check if we have available memory
    if (frames_used + nframes >= frames_pool_size)
    {
        panic("pmm: no physical memory left!");
        return (uintptr_t)NULL;
    }

    // Find free frame in the pool
    for (i = 0; i < frames_first_good_block + frames_pool_size; i++)
    {
        frame = &frames_pool[i];
        if (!(frame->flags & PMM_FRAME_AVAILABLE) || frame->flags & PMM_FRAME_USED) {
            block_size = 0;
            continue;
        }

        if (block_size == 0)
            block_offset = i;

        block_size++;
        if (block_size >= nframes)
            break;
    }

    if (block_size < nframes) {
        printk(KERN_WARNING "pmm: no available contiguous blocks were found (nframes = %d).", nframes);
        return (uintptr_t)NULL;
    }

    // Flag all blocks as used
    for (i = 0; i < block_size; i++) {
        frames_pool[block_offset + i].flags |= PMM_FRAME_USED;
    }

    frames_used += nframes;
    return frames_pool[block_offset].base;
}

int pmm_free_frames(uintptr_t frames, size_t nframes)
{
    size_t i;
    if (frames == (uintptr_t)NULL)
        return 1;

    for (i = 0; i < nframes; i++) {
        frames_pool[(frames >> 12) + i].flags &= ~PMM_FRAME_USED;
    }

    return 0;
}

size_t pmm_usage()
{
    return frames_used;
}

size_t pmm_total()
{
    return frames_pool_size;
}
