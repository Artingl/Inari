#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

#include <kernel/kernel.h>
#include <kernel/include/C/string.h>
#include <kernel/sys/console/console.h>

#include <drivers/memory/memory.h>
#include <drivers/video/video.h>
#include <drivers/video/vbe/vbe_drv.h>
#include <drivers/video/font.h>

struct kern_video video;
uint8_t video_loaded;

uint16_t *__font_map;

void video_init()
{
    size_t i;
    struct kernel_payload const *payload = kernel_configuration();
    memcpy(&video, &payload->video_service, sizeof(struct kern_video));
    video_loaded = 0;

    switch (video.mode)
    {
    case VIDEO_VBE:
    {
        printk("video: Using VBE");
        kernel_assert(vbe_init((struct kern_video_vbe*)video.info_structure) == 0, "video: vbe init failed");
        goto end;
    }
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");

end:
    video_loaded = 1;
    memory_forbid_region((uintptr_t)video_fb(), video_fb_size());

    // initialize characters map
    __font_map = kcalloc(sizeof(uint16_t), 0xffff);
    for (i = 0; i < 2899; i++)
    {
        __font_map[font.index[i]] = i * 16;
    }

    video_clear();
}

void video_text_print_at(uint8_t *buffer, size_t length, uint8_t bg, uint8_t fg, uint32_t x, uint32_t y)
{
    size_t i, j, buffer_offset;
    uint8_t char_byte, color;
    extern const struct bitmap_font font;

    if (!video_loaded) return;

    uint32_t width = video_width();
    uint32_t height = video_height();
    uint32_t text_width = width / font.width;
    uint32_t text_height = height / font.height;
    uint32_t bytes_offset = 0;
    x *= font.width;
    y *= font.height;

    while (buffer_offset < length)
    {
        uint16_t bitmap_offset = __font_map[font_parse_unicode((char*)&buffer[buffer_offset + bytes_offset], &bytes_offset)];

        for (i = 0; i < font.height; i++)
        {
            char_byte = font.bitmap[bitmap_offset + i];

            for (j = 0; j < font.width; j++)
            {
                color = char_byte & (1 << (7 - j)) ? fg : bg;
                video_put_pixel(x + j, y + i, color, color, color);
            }
        }

        buffer_offset++;
        x += font.width;
        if (x > width)
        {
            x = 0;
            y += font.height;
            if (y > height)
            {
                y = 0;
                return;
            }
        }
    }
    
}

void video_put_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b)
{
    if (!video_loaded) return;

    switch (video.mode)
    {
    case VIDEO_VBE:
    {
        vbe_put_pixel(x, y, r, g, b);
        return;
    }
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }
}

void video_setmode(uint8_t mode, uint32_t width, uint32_t height)
{
    if (!video_loaded) return;

    panic("video: video_setmode is not implemented.");
}

void video_clear()
{
    if (!video_loaded) return;

    switch (video.mode)
    {
    case VIDEO_VBE:
    {
        vbe_clear();
        return;
    }
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");
}

int video_width()
{
    if (!video_loaded) return 0;

    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_width();
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");
}

int video_height()
{
    if (!video_loaded) return 0;

    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_height();
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");
}

int video_bpp()
{
    if (!video_loaded) return 0;

    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_bpp();
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");
}

void *video_fb()
{
    if (!video_loaded) return NULL;

    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_fb();
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");
}

size_t video_fb_size()
{
    if (!video_loaded) return 0;

    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_fb_size();
    case VIDEO_GOP:
    {
        panic("video: GOP not implemented");
    }
    }

    panic("video: Invalid video mode!");
}