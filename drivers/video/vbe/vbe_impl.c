#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/serial/serial.h>
#include <drivers/memory/vmm.h>
#include <drivers/video/vbe/vbe_drv.h>
#include <drivers/impl.h>

#include <drivers/video/font.h>

struct kern_video_vbe vbe;

// todo: use heap for this
uint16_t __font_map[0xffff];

int vbe_init(struct kern_video_vbe *__vbe)
{
    size_t i;
    memcpy(&vbe, __vbe, sizeof(struct kern_video_vbe));

    if (strcmp(vbe.vbe_block.vbe_signature, "VESA") != 0)
    {
        printk(KERN_WARNING "VBE: invalid signature ('%4s' != 'VESA')", vbe.vbe_block.vbe_signature);
        return VBE_INVALID_SIGNATURE;
    }

    // map VBE framebuffer into memory
    kident(vbe_fb(), vbe_fb_size(), KERN_PAGE_RW);

    printk(KERN_DEBUG "VBE: width = %d, height = %d, bpp = %d, fb = %p (phys: %p)",
           vbe_width(), vbe_height(), vbe_bpp(),
           (unsigned long)vbe_fb(), (unsigned long)vmm_get_phys(vmm_current_directory(), vbe_fb()));

    // initialize characters map
    for (i = 0; i < 2899; i++)
    {
        __font_map[font.index[i]] = i * 16;
    }

    // vbe_set_at('A', 0x07, 4 * VBE_WIDTH_CHARS + 20);

    // vbe_print_at("Hello fucking world хуй", 0x07, 0);
    // __asm__ volatile("hlt");

    // for (size_t i  = 0; i < 0xffff; i++)
    // {
    //     if (font.bitmap[i] == __X_X_XX)
    //     {
    //         printk("-- %d", i);
    //     }
    // }

    return VBE_SUCCESS;
}

void vbe_clear()
{
    memset(vbe_fb(), 0, vbe_fb_size());
}

int vbe_print_at(uint32_t *buffer, uint8_t clr, size_t offset, size_t length)
{
    size_t i, j, buffer_offset;
    uint8_t char_byte, color;

    uint32_t x = (offset % vbe_text_width()) * font.width;
    uint32_t y = (offset / vbe_text_height()) * font.height;

    uint8_t background = 0x00;
    uint8_t foreground = 0xcc;

    while (buffer_offset < length)
    {
        uint16_t bitmap_offset = __font_map[font_parse_unicode((char*)&buffer[buffer_offset], NULL)];

        for (i = 0; i < font.height; i++)
        {
            char_byte = font.bitmap[bitmap_offset + i];

            for (j = 0; j < font.width; j++)
            {
                color = char_byte & (1 << (7 - j)) ? foreground : background;
                vbe_put_pixel(x + j, y + i, color, color, color);
            }
        }

        buffer_offset++;
        x += font.width;
        if (x > vbe.vbe_mode.info.width)
        {
            x = 0;
            y += font.height;
            if (y > vbe.vbe_mode.info.height)
            {
                y = 0;
            }
        }
    }
}

int vbe_set_at(char c, uint8_t clr, size_t offset)
{
    size_t i, j;
    uint8_t char_byte, color;

    uint32_t x = (offset % vbe_text_width()) * font.width;
    uint32_t y = (offset / vbe_text_height()) * font.height;

    uint16_t bitmap_offset = (c - 31) * 16;

    for (i = 0; i < font.height; i++)
    {
        char_byte = font.bitmap[bitmap_offset + i];

        for (j = 0; j < font.width; j++)
        {
            color = char_byte & (1 << j) ? 255 : 0;
            vbe_put_pixel(x + j, y + i, color, color, color);
        }
    }
}

int vbe_text_width()
{
    return vbe.vbe_mode.info.width / font.width;
}

int vbe_text_height()
{
    return vbe.vbe_mode.info.height / font.height;
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

void vbe_render_text_buffer(uint32_t *buffer, size_t width, size_t height)
{
    size_t i, j;
    uint32_t x = 0, y = 0;
    uint8_t char_byte, color;

    uint8_t background = 0x00;
    uint8_t foreground = 0xcc;

    for (x = 0; x < width; x++)
    {
        for (y = 0; y < height; y++)
        {
            uint16_t bitmap_offset = __font_map[font_parse_unicode((char*)&buffer[y * width + x], NULL)];

            for (i = 0; i < font.height; i++)
            {
                char_byte = font.bitmap[bitmap_offset + i];

                for (j = 0; j < font.width; j++)
                {
                    color = char_byte & (1 << (7 - j)) ? foreground : background;
                    vbe_put_pixel((x * font.width) + j, (y * font.height) + i, color, color, color);
                }
            }
        }
    }
}

void vbe_put_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < 0 || y < 0 || x >= vbe.vbe_mode.info.width || y >= vbe.vbe_mode.info.height)
        return;

    *((uint8_t *)vbe_fb() + y * (vbe_width() * 4) + (x * 4)) = r;
    *((uint8_t *)vbe_fb() + y * (vbe_width() * 4) + (x * 4) + 1) = g;
    *((uint8_t *)vbe_fb() + y * (vbe_width() * 4) + (x * 4) + 2) = b;
}

void *vbe_fb()
{
    return vbe.vbe_mode.info.framebuffer;
}

size_t vbe_fb_size()
{
    // todo: replace 4 with vbe_bpp()
    return vbe_width() * vbe_height() * 4;
}
