#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_VESA
#ifdef CONFIG_SUBSYS_VIDEO

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/vmm.h>
#include <kernel/module.h>
#include <kernel/subsys/video.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sys/device.h>
#include <kernel/timer.h>

#include <misc/string.h>

#include <arch/paging.h>
#include <arch/x86/cpu.h>

#define VESA_GET_INFO      0x4F00
#define VESA_GET_MODE_INFO 0x4F01
#define VESA_SET_MODE      0x4F02

#define EDID_GET_DATA 0x4f15

struct edid_timing_desc {
    uint8_t hz_freq;
    uint8_t vt_freq;
    uint8_t hz_active_tm;
    uint8_t hz_blanking_tm;
    uint8_t hz_active_blanking_tm;
    uint8_t vt_active_tm;
    uint8_t vt_blanking_tm;
    uint8_t vt_active_blanking_tm;
    uint8_t hz_sync_off;
    uint8_t hz_sync_pulse_width;
    uint8_t vt_sync_off_vt_sync_pulse_width;
    uint8_t vt_hz_sync_offset_pulse_width;
    uint8_t hz_image_sz;
    uint8_t vt_image_sz;
    uint8_t hz_image_sz_vt_image_sz;
    uint8_t hz_border;
    uint8_t vt_border;
    uint8_t type_of_display;
} __attribute__((packed));

struct edid_record {
    char padding[8];
    uint16_t manufacture_id;
    uint16_t id_code;
    uint32_t serial_number;
    uint8_t manufacture_week;
    uint8_t manufacture_year;
    uint8_t version;
    uint8_t revision;
    uint8_t video_inp_type;
    uint8_t max_hz_sz;
    uint8_t max_vt_sz;
    uint8_t gamma_factor;
    uint8_t dpms_flags;
    char chroma_information[10];
    uint8_t established_timings1;
    uint8_t established_timings2;
    uint8_t manufacture_reserved_timings;
    uint16_t timing_ident[8];
    struct edid_timing_desc desc1;
    struct edid_timing_desc desc2;
    struct edid_timing_desc desc3;
    struct edid_timing_desc desc4;
    uint8_t unused;
    uint8_t checksum;
} __attribute__((packed));

struct vesa_block_info {
    char vesa_signature[4];     // == "VESA"
    uint16_t vesa_version;      // == 0x0300 for VBE 3.0
    uint16_t oem_string_ptr[2]; // isa vesaFarPtr
    uint8_t capabilities[4];
    uint16_t video_mode_ptr[2]; // isa vesaFarPtr
    uint16_t total_memory;      // as # of 64KB blocks
    uint8_t reserved[492];
} __attribute__((packed));

struct vesa_mode_info {
    uint16_t mode_attr;    /* 0 */
    uint8_t win_attr[2];   /* 2 */
    uint16_t win_grain;    /* 4 */
    uint16_t win_size;     /* 6 */
    uint16_t win_seg[2];   /* 8 */
    uint32_t win_scheme;   /* 12 */
    uint16_t logical_scan; /* 16 */

    uint16_t h_res;        /* 18 */
    uint16_t v_res;        /* 20 */
    uint8_t char_width;    /* 22 */
    uint8_t char_height;   /* 23 */
    uint8_t memory_planes; /* 24 */
    uint8_t bpp;           /* 25 */
    uint8_t banks;         /* 26 */
    uint8_t memory_layout; /* 27 */
    uint8_t bank_size;     /* 28 */
    uint8_t image_planes;  /* 29 */
    uint8_t page_function; /* 30 */

    uint8_t rmask;     /* 31 */
    uint8_t rpos;      /* 32 */
    uint8_t gmask;     /* 33 */
    uint8_t gpos;      /* 34 */
    uint8_t bmask;     /* 35 */
    uint8_t bpos;      /* 36 */
    uint8_t resv_mask; /* 37 */
    uint8_t resv_pos;  /* 38 */
    uint8_t dcm_info;  /* 39 */

    uint32_t lfb_ptr;        /* 40 Linear frame buffer address */
    uint32_t offscreen_ptr;  /* 44 Offscreen memory address */
    uint16_t offscreen_size; /* 48 */

    uint8_t reserved[206]; /* 50 */
} __attribute__((packed));

static _lo_data struct edid_record edid_record;
static _lo_data struct vesa_block_info vesa_block;
static _lo_data struct vesa_mode_info lo_mode_info;

static uint32_t *current_mode_base = 0;
static uint32_t mode_id = 0;
static struct vesa_mode_info current_mode_info;

static spinlock_t vesa_lock = {0};
static int vesa_initialized = 0;
static dev_t vesa_dev = 0;

struct video_ops ops;

static int vesa_switch_mode(uint32_t x, uint32_t y, uint8_t bpp, uint8_t allow_similar) {
    struct x86_regs16 r = {0};
    struct vesa_mode_info *info = &lo_mode_info;
    uint16_t *modes, similar = 0xffff;
    uint32_t diff, last_diff = 0, best_diff = 0, cur_diff;
    size_t i;

    if (!vesa_initialized)
        return -EINVAL;

    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    /* Go through all modes */
    modes = (uint16_t *)REAL_PTR(vesa_block.video_mode_ptr);
    for (i = 0; modes[i] != 0xFFFF; i++) {
        r.ax = VESA_GET_MODE_INFO;
        r.cx = modes[i];
        r.es = SEG(info);
        r.di = OFF(info);
        v86_bios(0x10, &r);

        if (r.ax != 0x4f)
            continue;

        /* Check that this mode is linear buffer mode and packed/direct color mode */
        if ((info->mode_attr & 0x99) == 0x99 && (info->memory_layout == 4 || info->memory_layout == 6) &&
            info->memory_planes == 1) {
            if (x == info->h_res && y == info->v_res && bpp == info->bpp)
                goto found_mode;

            /* Allow to also search for similar modes */
            if (allow_similar) {
                diff = info->h_res * info->v_res;
                cur_diff = last_diff > diff ? (last_diff - diff) : (diff - last_diff);
                if (best_diff == 0 || cur_diff < best_diff) {
                    last_diff = diff;
                    best_diff = cur_diff;
                    similar = i;
                }
            }
        }
    }

    /* If found similar and allow to set it, use it */
    if (similar != 0xffff && allow_similar) {
        kprintf("vesa: no mode %dx%d_%d, using similar", x, y, bpp);
        i = similar;
        goto found_mode;
    }

    spin_unlock_irqrestore(&vesa_lock, flags);
    return -EINVAL;
found_mode:
    r.ax = VESA_GET_MODE_INFO;
    r.cx = modes[i];
    r.es = SEG(info);
    r.di = OFF(info);
    v86_bios(0x10, &r);
    if (r.ax != 0x4f) {
        spin_unlock_irqrestore(&vesa_lock, flags);
        return -1;
    }

    kprintf("vesa: swithing mode to %dx%d_%d", info->h_res, info->v_res, info->bpp);

    r.ax = VESA_SET_MODE;
    r.bx = modes[i] | 0x4000;
    v86_bios(0x10, &r);
    if (r.ax != 0x4f) {
        spin_unlock_irqrestore(&vesa_lock, flags);
        return -1;
    }

    /* Cleanup previous mode */
    if (current_mode_base) {
        arch_unmap_page(arch_get_kernel_pagedir(), (void *)current_mode_base, vesa_block.total_memory * 0x10000);

        vmm_free_pages(arch_get_kernel_pagedir(), (void *)current_mode_base, (vesa_block.total_memory * 0x10000) >> 12);
    }

    /* Save new mode and allocate virtual memory for it */
    memcpy((void *)&current_mode_info, (void *)info, sizeof(current_mode_info));
    current_mode_base = (uint32_t *)vmm_alloc_vmem_kern((vesa_block.total_memory * 0x10000) >> 12);
    arch_map_page(arch_get_kernel_pagedir(), (void *)current_mode_base, (void *)current_mode_info.lfb_ptr,
                  vesa_block.total_memory * 0x10000, PAGE_PRESENT | PAGE_RW);

    if (current_mode_base == NULL) {
        spin_unlock_irqrestore(&vesa_lock, flags);
        return -1;
    }

    if (vesa_dev)
        video_remove_device(vesa_dev);
    video_add_device(&vesa_dev, "vesa0", (uintptr_t)current_mode_base, &ops);
    mode_id = modes[i];

    spin_unlock_irqrestore(&vesa_lock, flags);
    return 0;
}

static int video_mode_find_next(struct video_device *device, struct video_mode_info *mode) {
    struct x86_regs16 r = {0};
    struct vesa_mode_info *info = &lo_mode_info;
    uint16_t *modes;
    uint32_t default_w = 0, default_h = 0;
    size_t i;

    if (!vesa_initialized)
        return -ENOSYS;
    if (!mode)
        return -EINVAL;

    int found = 0,
        // If mode values are 0, return any first found mode
        found_mode = mode->width == 0 || mode->height == 0 || mode->bpp == 0 || mode->mode_id == 0;

    /* Try tp fetch display info with edid */
    r.ax = EDID_GET_DATA;
    r.bx = 0x0001;
    r.cx = 0;
    r.dx = 0;
    r.es = SEG(&edid_record);
    r.di = OFF(&edid_record);
    v86_bios(0x10, &r);

    if (r.ax == 0x4f) {
        default_w = edid_record.desc1.hz_active_tm | ((int)(edid_record.desc1.hz_active_blanking_tm & 0xF0) << 4);
        default_h = edid_record.desc1.vt_active_tm | ((int)(edid_record.desc1.vt_active_blanking_tm & 0xF0) << 4);
    }

    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    modes = (uint16_t *)REAL_PTR(vesa_block.video_mode_ptr);
    for (i = 0; modes[i] != 0xFFFF; i++) {
        r.ax = VESA_GET_MODE_INFO;
        r.cx = modes[i];
        r.es = SEG(info);
        r.di = OFF(info);
        v86_bios(0x10, &r);

        if (r.ax != 0x4f)
            continue;
        if (mode->mode_id == modes[i] && !found_mode) {
            found_mode = 1;
            continue;
        }

        /* Check that this mode is linear buffer mode and packed/direct color mode */
        if ((info->mode_attr & 0x99) == 0x99 && (info->memory_layout == 4 || info->memory_layout == 6) &&
            info->memory_planes == 1) {
            if (found_mode) {
                mode->is_default = 0;
                if (default_w == info->h_res && default_h == info->v_res)
                    mode->is_default = 1;

                mode->width = info->h_res;
                mode->height = info->v_res;
                mode->bpp = info->bpp;
                mode->mode_id = modes[i];
                found = 1;
                break;
            }
        }
    }

    spin_unlock_irqrestore(&vesa_lock, flags);
    return found;
}

static int video_mode_info(struct video_device *device, struct video_mode_info *result) {
    if (!vesa_initialized)
        return -ENOSYS;
    if (!result)
        return -EINVAL;
    if (mode_id == 0)
        return -ENOSYS;

    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);
    result->width = current_mode_info.h_res;
    result->height = current_mode_info.v_res;
    result->bpp = current_mode_info.bpp;
    result->mode_id = mode_id;
    spin_unlock_irqrestore(&vesa_lock, flags);

    return 0;
}

static int video_mode_switch(struct video_device *device, struct video_mode_info *new_mode) {
    if (!vesa_initialized)
        return -ENOSYS;

    uint32_t flags;
    int res = vesa_switch_mode(new_mode->width, new_mode->height, new_mode->bpp, 0);
    /* Note: pointer to device after successful switch is invalid */
    if (res != 0)
        return res;

    /* Fetch info for the new mode */
    if (new_mode) {
        spin_lock_irqsave(&vesa_lock, flags);
        new_mode->width = current_mode_info.h_res;
        new_mode->height = current_mode_info.v_res;
        new_mode->bpp = current_mode_info.bpp;
        new_mode->mode_id = mode_id;
        spin_unlock_irqrestore(&vesa_lock, flags);
    }

    return 0;
}

int vesa_map_video(struct video_device *device, struct video_map_info *mmap) {
    if (!vesa_initialized)
        return -ENOSYS;
    if (!mmap)
        return -EINVAL;
    pagedir_t *dir = arch_get_pagedir();
    void *vmem;
    if ((vmem = vmm_alloc_vmem_user(dir, (vesa_block.total_memory * 0x10000) >> 12)) == NULL)
        return -ENOMEM;
    mmap->size = vesa_block.total_memory * 0x10000;
    mmap->base = (uint8_t *)arch_map_page(dir, vmem, (void *)current_mode_info.lfb_ptr, mmap->size,
                                          PAGE_USR | PAGE_PRESENT | PAGE_RW);
    return 0;
}

#pragma GCC push_options
#pragma GCC optimize("O3")
static int vesa_blit(struct video_device *device, struct video_blit *blit_info) {
    if (!vesa_initialized)
        return -ENOSYS;
    if (!blit_info || !device)
        return -EINVAL;

    int res = -EINVAL;
    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    if (!blit_info->buffer || !current_mode_base)
        goto end;
    if (video_format_bpp[blit_info->format] == 0)
        goto end;
    int32_t x, y;
    size_t blit_bpp = video_format_bpp[blit_info->format] >> 3;
    uint8_t r, g, b;
    uintptr_t size = vesa_block.total_memory * 0x10000, off, buf_off = 0,
              buf_size = blit_info->width * blit_info->height * blit_bpp;

    for (y = blit_info->y; y < blit_info->height + blit_info->y; y++)
        for (x = blit_info->x; x < blit_info->width + blit_info->x; x++) {
            if (y >= current_mode_info.v_res || x >= current_mode_info.h_res || x < 0 || y < 0)
                continue;
            off = y * current_mode_info.h_res + x;
            buf_off = (y - blit_info->y) * blit_bpp * blit_info->width + (x - blit_info->x) * blit_bpp;
            if (off >= size || buf_off + 2 >= buf_size)
                continue;
            r = blit_info->buffer[buf_off + 0];
            g = blit_info->buffer[buf_off + (blit_bpp > 1 ? 1 : 0)];
            b = blit_info->buffer[buf_off + (blit_bpp > 2 ? 2 : 0)];

            /* TODO: account for video mode bpp */
            current_mode_base[off] = (((((0xff << 8) | r) << 8) | g) << 8) | b;
        }

    res = 0;
end:
    spin_unlock_irqrestore(&vesa_lock, flags);
    return res;
}

static int vesa_fill_rect(struct video_device *device, struct video_fill_rect *rect) {
    if (!vesa_initialized)
        return -ENOSYS;
    if (!rect || !device)
        return -EINVAL;

    int res = -EINVAL;
    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    if (!current_mode_base || video_format_bpp[rect->format] == 0)
        goto end;
    int32_t x, y;
    uint8_t r, g, b;
    size_t rect_bpp = video_format_bpp[rect->format] >> 3;
    uintptr_t size = vesa_block.total_memory * 0x10000, off;

    for (y = rect->y; y < rect->height + rect->y; y++)
        for (x = rect->x; x < rect->width + rect->x; x++) {
            if (y >= current_mode_info.v_res || x >= current_mode_info.h_res || x < 0 || y < 0)
                continue;
            off = y * current_mode_info.h_res + x;
            if (off >= size)
                continue;
            r = rect_bpp > 2 ? rect->color >> 16 : (uint8_t)rect->color;
            g = rect_bpp > 2 ? (rect->color >> 8) & 0xff : (uint8_t)rect->color;
            b = rect_bpp > 2 ? rect->color & 0xff : (uint8_t)rect->color;

            /* TODO: account for video mode bpp */
            current_mode_base[off] = (((((0xff << 8) | r) << 8) | g) << 8) | b;
        }

    res = 0;
end:
    spin_unlock_irqrestore(&vesa_lock, flags);
    return res;
}
#pragma GCC pop_options

static int vesa_probe() {
    struct x86_regs16 r = {0};
    uint32_t x = 800, y = 600, bpp = 32;

    r.ax = VESA_GET_INFO;
    r.es = SEG(&vesa_block);
    r.di = OFF(&vesa_block);

    v86_bios(0x10, &r);
    if (r.ax != 0x4f)
        return -ENODEV;
    vesa_initialized = 1;

    /* Try tp fetch display info with edid */
    r.ax = EDID_GET_DATA;
    r.bx = 0x0001;
    r.cx = 0;
    r.dx = 0;
    r.es = SEG(&edid_record);
    r.di = OFF(&edid_record);
    v86_bios(0x10, &r);

    if (r.ax == 0x4f) {
        x = edid_record.desc1.hz_active_tm | ((int)(edid_record.desc1.hz_active_blanking_tm & 0xF0) << 4);
        y = edid_record.desc1.vt_active_tm | ((int)(edid_record.desc1.vt_active_blanking_tm & 0xF0) << 4);
    }
    mode_id = 0;
    vesa_switch_mode(x, y, bpp, 1);
    kprintf("vesa: initialized");
    return 0;
}

static void vesa_disable(struct video_device *device) {
    if (!vesa_initialized)
        return;
    struct x86_regs16 r = {0};
    // struct vesa_mode_info *info = &lo_mode_info;
    // uint16_t *modes;
    // size_t i;

    /* Switch back to 80x25 mode */
    r.ax = 0x0003; // VESA_SET_MODE;
    // r.bx = modes[i];
    v86_bios(0x10, &r);
    // modes = (uint16_t *)REAL_PTR(vesa_block.video_mode_ptr);
    // for (i = 0; modes[i] != 0xFFFF; i++) {
    //     r.ax = VESA_GET_MODE_INFO;
    //     r.cx = modes[i];
    //     r.es = SEG(info);
    //     r.di = OFF(info);
    //     v86_bios(0x10, &r);

    //     if (r.ax != 0x4f)
    //         continue;

    //     if ((info->mode_attr & 0x15) == 0x05 && info->h_res == 80 && info->v_res == 25) {
    //         r.ax = VESA_SET_MODE;
    //         r.bx = modes[i];
    //         v86_bios(0x10, &r);
    //         break;
    //     }
    // }
}

static void vesa_cleanup() {
    if (!vesa_initialized)
        return;
    kprintf("vesa: cleaning up");

    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    if (vesa_dev)
        video_remove_device(vesa_dev);
    vesa_dev = 0;
    mode_id = 0;

    /* Cleanup current mode */
    if (current_mode_base) {
        arch_unmap_page(arch_get_kernel_pagedir(), (void *)current_mode_base, vesa_block.total_memory * 0x10000);

        vmm_free_pages(arch_get_kernel_pagedir(), (void *)current_mode_base, (vesa_block.total_memory * 0x10000) >> 12);
        current_mode_base = NULL;
    }

    vesa_disable(NULL);
    vesa_initialized = 0;

    spin_unlock_irqrestore(&vesa_lock, flags);
}

struct video_ops ops = {.mode_info = &video_mode_info,
                        .mode_find_next = &video_mode_find_next,
                        .mode_switch = &video_mode_switch,
                        .blit = &vesa_blit,
                        .disable = &vesa_disable,
                        .fill_rect = &vesa_fill_rect,
                        .map_video = &vesa_map_video};

module_t vesa_module = {
    .probe = vesa_probe,
    .cleanup = vesa_cleanup,
    .flags = MODULE_FLAG_LAZY_LOAD,
};

module_register("vesa", vesa_module);

#endif
#endif
#endif