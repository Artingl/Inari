#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_VESA

#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/module.h>
#include <kernel/sync/spinlock.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/mm/vmm.h>

#include <misc/string.h>

#include <arch/paging.h>
#include <arch/x86/cpu.h>

#define VESA_GET_INFO       0x4F00
#define VESA_GET_MODE_INFO  0x4F01
#define VESA_SET_MODE       0x4F02

#define EDID_GET_DATA 0x4f15

struct edid_timing_desc
{
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
} __attribute__ ((packed));

struct edid_record
{
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
} __attribute__ ((packed));

struct vesa_block_info {
   char     vesa_signature[4];         // == "VESA"
   uint16_t vesa_version;              // == 0x0300 for VBE 3.0
   uint16_t oem_string_ptr[2];         // isa vesaFarPtr
   uint8_t  capabilities[4];
   uint16_t video_mode_ptr[2];         // isa vesaFarPtr
   uint16_t total_memory;             // as # of 64KB blocks
   uint8_t  reserved[492];
} __attribute__((packed));

struct vesa_mode_info {
	uint16_t mode_attr;		/* 0 */
	uint8_t win_attr[2];		/* 2 */
	uint16_t win_grain;		/* 4 */
	uint16_t win_size;		/* 6 */
	uint16_t win_seg[2];		/* 8 */
	uint32_t win_scheme;	/* 12 */
	uint16_t logical_scan;	/* 16 */

	uint16_t h_res;		/* 18 */
	uint16_t v_res;		/* 20 */
	uint8_t char_width;		/* 22 */
	uint8_t char_height;		/* 23 */
	uint8_t memory_planes;	/* 24 */
	uint8_t bpp;			/* 25 */
	uint8_t banks;		/* 26 */
	uint8_t memory_layout;	/* 27 */
	uint8_t bank_size;		/* 28 */
	uint8_t image_planes;	/* 29 */
	uint8_t page_function;	/* 30 */

	uint8_t rmask;		/* 31 */
	uint8_t rpos;		/* 32 */
	uint8_t gmask;		/* 33 */
	uint8_t gpos;		/* 34 */
	uint8_t bmask;		/* 35 */
	uint8_t bpos;		/* 36 */
	uint8_t resv_mask;		/* 37 */
	uint8_t resv_pos;		/* 38 */
	uint8_t dcm_info;		/* 39 */

	uint32_t lfb_ptr;		/* 40 Linear frame buffer address */
	uint32_t offscreen_ptr;	/* 44 Offscreen memory address */
	uint16_t offscreen_size;	/* 48 */

	uint8_t reserved[206];	/* 50 */
} __attribute__ ((packed));

static _lo_data struct edid_record edid_record;
static _lo_data struct vesa_block_info vesa_block;
static _lo_data struct vesa_mode_info lo_mode_info;

static uint32_t *current_mode_base = 0;
static struct vesa_mode_info current_mode_info;

static spinlock_t vesa_lock = {0};
static int vesa_initialized = 0;


#define IOCTL_MODE_SWITCH   0
#define IOCTL_MODE_GET      1
#define IOCTL_BLIT          2

struct drv_blit
{
#define VESA_BLIT_R8G8B8_FORMAT   0
    uint8_t format;
    uint8_t *buffer;
    uint32_t x, y;
    uint32_t width, height;
};

struct drv_mode_info
{
    uint32_t width, height;
    uint8_t bpp, allow_similar;
};

struct drv_fetch_mode_info
{
    uint16_t idx;

    struct {
        uint32_t width;
        uint32_t height;
        uint8_t bpp;
    } mode_info;
};

static int vesa_switch_mode(uint32_t x, uint32_t y, uint8_t bpp, uint8_t allow_similar)
{
    struct x86_regs16 r;
    struct vesa_mode_info *info = &lo_mode_info;
    uint16_t *modes, similar = 0xffff;
    uint32_t diff, last_diff = 0, best_diff = 0, cur_diff;
    size_t i;

    if (!vesa_initialized) return -EINVAL;

    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    /* Go through all modes */
    modes = (uint16_t*)REAL_PTR(vesa_block.video_mode_ptr);
    for (i = 0 ; modes[i] != 0xFFFF ; i++)
    {
        r.ax = VESA_GET_MODE_INFO;
        r.cx = modes[i];
        r.es = SEG(info);
        r.di = OFF(info);
        v86_bios(0x10, &r);
        
        if (r.ax != 0x4f) continue;

        /* Check that this mode is linear buffer mode and packed/direct color mode */
        if ((info->mode_attr & 0x99) == 0x99 &&
			   (info->memory_layout == 4 ||
			    info->memory_layout == 6) &&
			   info->memory_planes == 1)
        {
            if (x == info->h_res && y == info->v_res && bpp == info->bpp) goto found_mode;

            /* Allow to also search for similar modes */
            if (allow_similar)
            {
                diff = info->h_res * info->v_res;
                cur_diff = last_diff > diff ? (last_diff - diff) : (diff - last_diff);
                if (best_diff == 0 || cur_diff < best_diff)
                {
                    last_diff = diff;
                    best_diff = cur_diff;
                    similar = i;
                }
            }
        }
    }

    /* If found similar and allow to set it, use it */
    if (similar != 0xffff && allow_similar)
    {
        printk("vesa: no mode %dx%d_%d, using similar", x, y, bpp);
        i = similar;
        goto found_mode;
    }

    spin_unlock_irqrestore(&vesa_lock, flags);
    return -1;
found_mode:
    r.ax = VESA_GET_MODE_INFO;
    r.cx = modes[i];
    r.es = SEG(info);
    r.di = OFF(info);
    v86_bios(0x10, &r);
    if (r.ax != 0x4f)
    {
        spin_unlock_irqrestore(&vesa_lock, flags);
        return -1;
    }

    printk("vesa: swithing mode to %dx%d_%d", info->h_res, info->v_res, info->bpp);

    r.ax = VESA_SET_MODE;
    r.bx = modes[i] | 0x4000;
    v86_bios(0x10, &r);
    if (r.ax != 0x4f)
    {
        spin_unlock_irqrestore(&vesa_lock, flags);
        return -1;
    }

    /* Cleanup previous mode */
    if (current_mode_base)
    {
        arch_unmap_page(
            arch_get_kernel_pagedir(),
            (void*)current_mode_base,
            vesa_block.total_memory * 0x10000
        );

        vmm_free_pages(
            arch_get_kernel_pagedir(),
            (void*)current_mode_base,
            (vesa_block.total_memory * 0x10000) >> 12
        );
    }

    /* Save new mode and allocate virtual memory for it */
    memcpy((void*)&current_mode_info, (void*)info, sizeof(current_mode_info));
    current_mode_base = (uint32_t*)vmm_alloc_vmem_kern((vesa_block.total_memory * 0x10000) >> 12);
    arch_map_page(
        arch_get_kernel_pagedir(),
        (void*)current_mode_base,
        (void*)current_mode_info.lfb_ptr,
        vesa_block.total_memory * 0x10000,
        PAGE_PRESENT | PAGE_RW
    );

    if (current_mode_base == NULL)
    {
        spin_unlock_irqrestore(&vesa_lock, flags);
        return -1;
    }

    spin_unlock_irqrestore(&vesa_lock, flags);
    return 0;
}

#pragma GCC push_options
#pragma GCC optimize("O3")

static int vesa_blit(struct drv_blit *blit_info)
{
    int res = -EINVAL;
    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);

    if (!blit_info->buffer || !current_mode_base) goto end;
    if (blit_info->format != VESA_BLIT_R8G8B8_FORMAT) goto end;
    size_t x, y;
    uintptr_t size = vesa_block.total_memory * 0x10000, off,
        buf_off = 0,
        // 3 in the end because VESA_BLIT_R8G8B8_FORMAT
        buf_size = blit_info->width * blit_info->height * 3;

    for (y = blit_info->y; y < blit_info->height + blit_info->y; y++)
        for (x = blit_info->x; x < blit_info->width + blit_info->x; x++)
        {
            off = y * current_mode_info.h_res + x;
            buf_off = 0; /* TODO: accessing `blit_info->buffer` causes page fault */
            if (off >= size || buf_off + 2 >= buf_size)
                continue;
            current_mode_base[off] = (((((0xff << 8) | blit_info->buffer[buf_off++]) << 8) | blit_info->buffer[buf_off++]) << 8) | blit_info->buffer[buf_off++];
        }
    
    res = 0;
end:
    spin_unlock_irqrestore(&vesa_lock, flags);
    return res;
}

#pragma GCC pop_options

static int vesa_ioctl(struct device *chardev, unsigned long req, void *arg)
{
    struct drv_mode_info *mode_info = (struct drv_mode_info *)arg;
    struct drv_fetch_mode_info *fetch_info = (struct drv_fetch_mode_info *)arg;
    struct drv_blit *blit_info = (struct drv_blit *)arg;
    if (!vesa_initialized) return -ENOSYS;
    if (!arg) return -EINVAL;

    switch (req)
    {
    case IOCTL_MODE_SWITCH:
        return vesa_switch_mode(mode_info->width, mode_info->height, mode_info->bpp, mode_info->allow_similar);

    case IOCTL_BLIT:
        return vesa_blit(blit_info);

    // case IOCTL_MODE_GET:

    }

    return -ENOSYS;
}

static dev_t vesa_chardev;
struct char_ops ops = {
    .ioctl = &vesa_ioctl
};

static int vesa_probe()
{
    struct x86_regs16 r;
    uint32_t x, y, bpp = 32;

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

    if (r.ax == 0x4f)
    {
        x = edid_record.desc1.hz_active_tm | ((int) (edid_record.desc1.hz_active_blanking_tm & 0xF0) << 4);
        y = edid_record.desc1.vt_active_tm | ((int) (edid_record.desc1.vt_active_blanking_tm & 0xF0) << 4);
    }

    register_chardev(VIDEO_DRIVER, &ops, NULL, &vesa_chardev);

    vesa_switch_mode(x, y, bpp, 1);
    printk("vesa: initialized");
    return 0;
}

static void vesa_cleanup()
{
    printk("vesa: cleaning up");

    struct x86_regs16 r;
    struct vesa_mode_info *info = &lo_mode_info;
    uint16_t *modes;
    size_t i;

    uint32_t flags;
    spin_lock_irqsave(&vesa_lock, flags);
    vesa_initialized = 0;
    unregister_chardev(vesa_chardev);

    /* Cleanup current mode */
    if (current_mode_base)
    {
        arch_unmap_page(
            arch_get_kernel_pagedir(),
            (void*)current_mode_base,
            vesa_block.total_memory * 0x10000
        );

        vmm_free_pages(
            arch_get_kernel_pagedir(),
            (void*)current_mode_base,
            (vesa_block.total_memory * 0x10000) >> 12
        );
        current_mode_base = NULL;
    }

    /* Switch back to 80x25 mode */
    modes = (uint16_t*)REAL_PTR(vesa_block.video_mode_ptr);
    for (i = 0 ; modes[i] != 0xFFFF ; i++)
    {
        r.ax = VESA_GET_MODE_INFO;
        r.cx = modes[i];
        r.es = SEG(info);
        r.di = OFF(info);
        v86_bios(0x10, &r);
        
        if (r.ax != 0x4f) continue;

        if ((info->mode_attr & 0x15) == 0x05 && info->h_res == 80 && info->v_res == 25) {
            r.ax = VESA_SET_MODE;
            r.bx = modes[i];
            v86_bios(0x10, &r);
            break;
        }
    }

    spin_unlock_irqrestore(&vesa_lock, flags);
}

module_t vesa_module = {
    .probe = vesa_probe,
    .cleanup = vesa_cleanup,
    .flags = MODULE_LAZY_LOAD,
};


module_register(
    "vesa",
    vesa_module
);

#endif
#endif