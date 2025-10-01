#ifndef _INARI_PMM_H
#define _INARI_PMM_H

#include <stddef.h>
#include <stdint.h>

#include <kernel/inari.h>

#define PMM_POOL_SIZE  262144

#define PMM_FRAME_AVAILABLE (1 << 0)
#define PMM_FRAME_DISABLED  (1 << 1)
#define PMM_FRAME_USED      (1 << 2)

typedef struct pmm_frame {
    uint8_t flags;
} pmm_frame_t;

extern int pmm_init(void);
extern void pmm_reserve_memory(struct reserved_memory region);
extern uintptr_t pmm_alloc_frames(size_t nframes);
extern int pmm_free_frames(uintptr_t frames, size_t nframes);
extern size_t pmm_usage();
extern size_t pmm_total();

#endif