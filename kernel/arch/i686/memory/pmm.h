#pragma once

#include <kernel/include/typedefs.h>

#define PMM_POOL_SIZE 1048576 // 256 * 4096   (POOL_FRAME_SIZE * 64 == 1MB)

#define PMM_FRAME_NOT_AVAILABLE (1 << 0)
#define PMM_FRAME_AVAILABLE     (1 << 1)
#define PMM_FRAME_USED          (1 << 2)

struct page_frame
{
    uint32_t base;
    uint8_t flags;
};


int pmm_init(struct kernel_mmap_entry *mmap_list, size_t mmap_list_length);

size_t pmm_usage();
size_t pmm_total();

uintptr_t pmm_alloc_frames(size_t nframes);
int pmm_free_frames(uintptr_t frames, size_t nframes);

int pmm_disable_region(uintptr_t origin, size_t size);
