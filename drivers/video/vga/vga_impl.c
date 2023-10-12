#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/video/vga/vga_drv.h>

bool vga_loaded = false;
struct kern_video_vga vga;

// TODO: invalid value in __vga given by the bootloader.
int vga_init(struct kern_video_vga *__vga)
{
    memcpy(&vga, __vga, sizeof(struct kern_video_vga));
    printk(KERN_DEBUG "VIDEO: using VGA %dx%d at %p (pointer: %p)", vga.rows, vga.columns, (unsigned long)vga.base, (unsigned long)__vga);
    vga_loaded = true;
    return 0;
}

void vga_clear()
{
    unsigned char *VIDEO_MEM = (unsigned char *)(vga_fb());
    size_t i;

    for (i = 0; i < vga_fb(); i++)
    {
        *(VIDEO_MEM++) = 0;
        *(VIDEO_MEM++) = 0;
    }
}

int vga_print_at(uint32_t *message, uint8_t clr, size_t offset, size_t length)
{
    unsigned char *VIDEO_MEM = (unsigned char *)(vga_fb() + offset * 2);
    size_t i;
    uint8_t c;

    while (i < length)
    {
        *(VIDEO_MEM++) = c;
        *(VIDEO_MEM++) = clr;
        i++;
    }

    return i;
}

void vga_render(uint32_t *buffer, size_t width, size_t height)
{
    unsigned char *VIDEO_MEM = (unsigned char *)(vga_fb());
    size_t i;
    uint8_t c;

    while (i < width * height)
    {
        *(VIDEO_MEM++) = (uint8_t)*(buffer++);
        *(VIDEO_MEM++) = 0x07;
        i++;
    }
}

int vga_set_at(char c, uint8_t clr, size_t offset)
{
    unsigned char *VIDEO_MEM = (unsigned char *)(vga_fb() + offset * 2);
    *(VIDEO_MEM++) = c;
    *(VIDEO_MEM++) = clr;
    return 1;
}

int vga_rows()
{
    if (vga_loaded)
        return vga.rows;

    return 80;
}

int vga_columns()
{
    if (vga_loaded)
        return vga.columns;

    return 25;
}

void *vga_fb()
{
    if (vga_loaded)
        return vga.base;

    return 0xb8000;
}

size_t vga_fb_size()
{
    return vga_rows() * vga_columns() * 2;
}
