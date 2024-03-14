#pragma once

#include <kernel/include/C/typedefs.h>

#define PMM_POOL_SIZE 1048576 // 256 * 4096   (POOL_FRAME_SIZE * 64 == 1MB)

#define PMM_FRAME_AVAILABLE (1 << 0)
#define PMM_FRAME_USED      (1 << 1)

struct page_frame
{
    uint32_t base;
    uint8_t flags;
};

void pmm_init();

uintptr_t pmm_alloc_frames(size_t nframes);
size_t pmm_usage();
size_t pmm_total();

int pmm_free_frames(uintptr_t frames, size_t nframes);

