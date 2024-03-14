#include <kernel/kernel.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

#include <drivers/video/video.h>
#include <drivers/memory/pmm.h>

#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>

#include <bootloader/lower.h>

struct page_frame frames_pool[PMM_POOL_SIZE];
size_t frames_pool_size = 0;
size_t frames_used = 0;

#define MAKE_FRAME(addr, f)                                                          \
    {                                                                                \
        frames_pool[(addr) >> 12] = (struct page_frame){.base = (addr), .flags = f}; \
        frames_pool_size++;                                                          \
    }

#define CHECK_OVERLAP(x0, x1, v) ((x0) <= (v) && (x1) >= (v))

bool __pmm_check_overlap(uintptr_t addr, size_t *offset);

void pmm_init()
{
    size_t i, j, offset = 0, addr, len;
    struct kernel_payload const *payload = kernel_configuration();
    struct kernel_mmap_entry *mmap;

    printk(KERN_DEBUG "pmm: memory maps base = %p, len = %d", (unsigned int)payload->mmap, payload->mmap_length);
    printk(KERN_DEBUG "pmm: page_frame_t base = %p, size = %x", (unsigned int)&frames_pool[0], (unsigned int)sizeof(frames_pool));

    memset(&frames_pool[0], 0, sizeof(frames_pool));

    printk("pmm: making frames");
    for (i = 0; i < payload->mmap_length; i++)
    {
        mmap = &payload->mmap[i];
        offset = 0;

        if (mmap->type & KERN_MMAP_AVAILABLE)
        {
            addr = align(mmap->addr, PAGE_SIZE);
            len = mmap->len;

            // Ignore memory under 1MB.
            if (addr < 0x100000)
            {
                addr = align(0x100000, PAGE_SIZE);
                if (len < align(0x100000, PAGE_SIZE))
                    continue;
            }

            // check for overlaps
            if (__pmm_check_overlap(addr, &offset))
            {
                printk(KERN_DEBUG "pmm: memory overlap: base = %p, size = %p, type = %p. Starting form offset %p",
                       (unsigned int)addr, (unsigned int)len, (unsigned int)mmap->type, (unsigned int)offset);
            }

            offset = align(offset, PAGE_SIZE);

            // allocate frames for found available memory
            for (
                j = addr + offset;
                j + PAGE_SIZE < addr + len && j > 0;
                j += PAGE_SIZE)
            {
                MAKE_FRAME(j, PMM_FRAME_AVAILABLE);
            }
        }
    }
}

extern void *_kernel_phys_start;
extern void *_kernel_phys_end;

extern void *_lo_start_marker;
extern void *_lo_end_marker;

bool __pmm_check_overlap(uintptr_t addr, size_t *offset)
{
    uintptr_t kernel_start = (uintptr_t)&_kernel_phys_start;
    uintptr_t kernel_end = (uintptr_t)&_kernel_phys_end;
    uintptr_t kernel_length = kernel_end - kernel_start;

    uintptr_t bootloader_start = (uintptr_t)&_lo_start_marker;
    uintptr_t bootloader_end = (uintptr_t)&_lo_end_marker;
    uintptr_t bootloader_length = bootloader_end - bootloader_start;

    if (addr < kernel_end)
    {
        *offset = kernel_end;
        return true;
    }

#if 0
    size_t highest_overlap = 0, overlap = 0;

    // we need to get consumed memory by the bootloader,
    // so we would not be able to accidentally overwrite it
    struct early_alloc_info early_memory = early_alloc_info();

    if (CHECK_OVERLAP(early_memory.heap_top, early_memory.heap, addr))
    {
        overlap = align(early_memory.heap - early_memory.heap_top, PAGE_SIZE);
        if (highest_overlap < overlap)
            highest_overlap = overlap;
    }
    if (CHECK_OVERLAP(video_fb(), video_fb() + video_fb_size(), addr))
    {
        overlap = align(video_fb_size(), PAGE_SIZE);
        if (highest_overlap < overlap)
            highest_overlap = overlap;
    }
    if (CHECK_OVERLAP(kernel_start, kernel_start + kernel_length, addr))
    {
        overlap = align(kernel_length, PAGE_SIZE);
        if (highest_overlap < overlap)
            highest_overlap = overlap;
    }
    if (CHECK_OVERLAP(bootloader_start, bootloader_start + bootloader_length, addr))
    {
        overlap = align(bootloader_length, PAGE_SIZE);
        if (highest_overlap < overlap)
            highest_overlap = overlap;
    }

    // TODO: check overlap on LAPIC and IO/APIC for all cores
    // if (CHECK_OVERLAP(cpu_io_apic_get_base() - PAGE_SIZE, cpu_io_apic_get_base() + PAGE_SIZE * 2, addr) ||
    //          CHECK_OVERLAP(cpu_lapic_get_base(), cpu_lapic_get_base() + PAGE_SIZE * 2, addr))
    // {
    //     overlap = PAGE_SIZE * 3;
    //     if (highest_overlap < overlap)
    //         highest_overlap = overlap;
    // }

    if (highest_overlap != 0) {
        *offset = highest_overlap;
        return true;
    }

    return false;
#endif
}

uintptr_t pmm_alloc_frames(size_t nframes)
{
    size_t i, j;
    if (frames_used + nframes >= frames_pool_size)
    {
        // no memory left
        panic("pmm: No physical memory left!");
        return (uintptr_t)NULL;
    }

    // iterate thru all available frames
    for (i = 0; i < frames_pool_size; i += nframes)
    {
        // check nframes at a time (if one of them has USED bit set, check next 4 blocks)
        j = i;
        for (j = 0; j < nframes; j++)
        {
            struct page_frame *frame = &frames_pool[i + j];

            if (frame->flags & PMM_FRAME_USED || !(frame->flags & PMM_FRAME_AVAILABLE))
                break;

            // this frame and all previous are free and can be used. return them
            if (j == nframes - 1)
            {
                goto found_frames;
            }
        }
    }

    panic("pmm: No available contiguous blocks were found (nframes = %d).", nframes);
    return (uintptr_t)NULL;

found_frames:
    // mark all found frames as used
    for (j = 0; j < nframes; j++)
    {
        frames_pool[i + j].flags |= PMM_FRAME_USED;
    }

    if (frames_pool[i].base == NULL)
        panic("pmm: Found NULL frame known as available.");

    frames_used += nframes;
    return frames_pool[i].base;
}

int pmm_free_frames(uintptr_t frames, size_t nframes)
{
    size_t i;
    for (i = 0; i < nframes; i++)
    {
        frames_pool[(frames >> 12) + i].flags &= ~PMM_FRAME_USED;
    }

    frames_used -= nframes;
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
