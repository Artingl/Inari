#include <kernel/inari.h>
#include <kernel/mm/pmm.h>

#include <misc/string.h>

// TODO: The pmm should be bootloader-agnostic
#include <multiboot/multiboot.h>

// TODO: That's not ideal. It'd be better if we allocated the pool dynamically.
//       Right now with the current pool size only 1gig of memory is addressable.
static struct pmm_frame frames_pool[PMM_POOL_SIZE];
static size_t available_frames = 0, frames_used = 0, first_good_frame = 0;

extern char kern_virt_end;

static int pmm_check_overlap(uintptr_t addr, size_t len)
{
    uintptr_t kernel_start = 0;
    uintptr_t kernel_end   = (uintptr_t)&kern_virt_end - KERN_VIRTUAL_ADDR;

    if (!(addr + len <= kernel_start || addr >= kernel_end))
        return -1;
    return 0;
}

int pmm_init(void)
{
    bootinfo_t info = get_boot_info();
    multiboot_info_t *multiboot = (multiboot_info_t*)info.bootloader_info;

    // TODO: The memory map should be bootloader-agnostic
    size_t i, j, len, mmap_length = multiboot->mmap_length / sizeof(struct multiboot_mmap_entry);
    uintptr_t addr;
    struct multiboot_mmap_entry *entry;
    for (i = 0; i < mmap_length; i++)
    {
        // Parse the memory map entry from the list provided by multiboot
        entry = &((struct multiboot_mmap_entry *)multiboot->mmap_addr)[i];
        addr = ALIGN(entry->addr, KERN_PAGE_SIZE);
        len = entry->len;

        // Populate the pool only with those regions that are available.
        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            for (j = addr; j < addr + len; j += KERN_PAGE_SIZE)
            {
                // Ensure that this memory region doesn't overlap important memory regions
                if (pmm_check_overlap(j, KERN_PAGE_SIZE) != 0)
                {
                    frames_pool[j / KERN_PAGE_SIZE].flags = PMM_FRAME_DISABLED;
                    continue;
                }

                // Check that we're not over the max pool size
                if (j >> 12 >= PMM_POOL_SIZE)
                {
                    printk("pmm: warning - over the pool size; %ld > %ld", j / KERN_PAGE_SIZE, PMM_POOL_SIZE);
                    goto end;
                }

                if (!(frames_pool[j / KERN_PAGE_SIZE].flags & PMM_FRAME_DISABLED))
                {
                    frames_pool[j / KERN_PAGE_SIZE].flags = PMM_FRAME_AVAILABLE;
                    available_frames++;

                    if (first_good_frame == 0 || first_good_frame > j / KERN_PAGE_SIZE)
                        first_good_frame = j / KERN_PAGE_SIZE;
                }
            }
        }
        else
        {
            pmm_reserve_memory((struct reserved_memory){
                .start = addr,
                .end = addr + len
            });
        }

    }

end:
    pmm_reserve_memory((struct reserved_memory){
        .start = 0,
        .end = (uintptr_t)&kern_virt_end - KERN_VIRTUAL_ADDR + KERN_PAGE_SIZE
    });

    printk("pmm: initialized, %lu frames available, %lukb.", available_frames, available_frames * (KERN_PAGE_SIZE / 1024));
    return 0;
}

void pmm_reserve_memory(struct reserved_memory region)
{
    printk("pmm: reserving region [0x%08x...0x%08x]", region.start, region.end);

    size_t i;
    for (i = region.start; i < region.end; i += KERN_PAGE_SIZE)
    {
        if (frames_pool[i / KERN_PAGE_SIZE].flags & PMM_FRAME_AVAILABLE)
        {
            available_frames--;
        }

        frames_pool[i / KERN_PAGE_SIZE].flags = PMM_FRAME_DISABLED;
    }
}

uintptr_t pmm_alloc_frames(size_t nframes)
{
    struct pmm_frame *frame;
    size_t i, block_offset = 0, block_size = 0;

    if (nframes == 0)
        return (uintptr_t)NULL;

    // Check if we have available memory
    if (frames_used + nframes > available_frames)
    {
        panic("pmm: no physical memory left!");
        return (uintptr_t)NULL;
    }

    // Find free frame in the pool
    for (i = first_good_frame; i < PMM_POOL_SIZE; i++)
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
        panic("pmm: no available contiguous blocks were found (nframes = %d).", nframes);
        return (uintptr_t)NULL;
    }

    // Flag all blocks as used
    for (i = 0; i < block_size; i++) {
        frames_pool[block_offset + i].flags |= PMM_FRAME_USED;
    }

    frames_used += nframes;
    return block_offset * KERN_PAGE_SIZE;
}

int pmm_free_frames(uintptr_t frames, size_t nframes)
{
    size_t i;
    if (frames == (uintptr_t)NULL)
        return 1;

    for (i = 0; i < nframes; i++) {
        frames_pool[(frames / KERN_PAGE_SIZE) + i].flags &= ~PMM_FRAME_USED;
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
    return available_frames;
}