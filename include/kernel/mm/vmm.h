#ifndef _INARI_VMM_H
#define _INARI_VMM_H

#include <stddef.h>
#include <stdint.h>

#include <kernel/inari.h>

#define VMM_FRAME_AVAILABLE (1 << 0)
#define VMM_FRAME_DISABLED  (1 << 1)
#define VMM_FRAME_USED      (1 << 2)

typedef struct vmm_frame {
    uint8_t flags;
} vmm_frame_t;

#define VMM_SIZE_BYTES   (sizeof(struct vmm_frame) * (0xffffffff / KERN_PAGE_SIZE))
#define VMM_SIZE_FRAMES  (VMM_SIZE_BYTES / KERN_PAGE_SIZE)
#define VMM_VBASE        (KERN_VIRTUAL_ADDR - VMM_SIZE_BYTES)

extern int vmm_init(void);
extern uintptr_t vmm_alloc_frames(size_t nframes);
extern int vmm_free_frames(uintptr_t frames, size_t nframes);

#endif