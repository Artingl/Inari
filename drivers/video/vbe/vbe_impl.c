#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/serial/serial.h>
#include <drivers/memory/vmm.h>
#include <drivers/video/vbe/vbe_drv.h>
#include <drivers/impl.h>

struct kern_video_vbe vbe;

int vbe_init(struct kern_video_vbe *__vbe)
{
    size_t i;
    memcpy(&vbe, __vbe, sizeof(struct kern_video_vbe));

    if (strcmp(vbe.vbe_block.vbe_signature, "VESA") != 0)
    {
        printk(KERN_WARNING "vbe: invalid signature ('%4s' != 'VESA')", vbe.vbe_block.vbe_signature);
        return VBE_INVALID_SIGNATURE;
    }

    // map VBE framebuffer into memory
    kident(vbe_fb(), vbe_fb_size(), KERN_PAGE_RW);

    printk("vbe: width = %d, height = %d, bpp = %d, fb = %p (phys: %p)",
           vbe_width(), vbe_height(), vbe_bpp(),
           (unsigned long)vbe_fb(), (unsigned long)vmm_get_phys(vmm_current_directory(), vbe_fb()));

    return VBE_SUCCESS;
}

void vbe_clear()
{
    memset(vbe_fb(), 0, vbe_fb_size());
}

int vbe_width()
{
    return vbe.vbe_mode.info.width;
}

int vbe_height()
{
    return vbe.vbe_mode.info.height;
}

int vbe_bpp()
{
    return vbe.vbe_mode.info.bpp;
}

void vbe_put_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uintptr_t offset = y * (vbe_width() * 4) + (x * 4);
    if (offset < 0 || offset > vbe_fb_size())
        return;

    *((uint8_t *)vbe_fb() + offset) = b;
    *((uint8_t *)vbe_fb() + offset + 1) = g;
    *((uint8_t *)vbe_fb() + offset + 2) = r;
}

void *vbe_fb()
{
    return (void*)vbe.vbe_mode.info.framebuffer;
}

size_t vbe_fb_size()
{
    // todo: replace 4 with vbe_bpp()
    return vbe_width() * vbe_height() * 4;
}
