#include <kernel/kernel.h>
#include <kernel/include/C/string.h>
#include <kernel/sys/console/console.h>

#include <drivers/video/video.h>

#include <drivers/video/vga/vga_drv.h>
#include <drivers/video/vbe/vbe_drv.h>

struct kern_video video;

void video_init()
{
    struct kernel_payload const *payload = kernel_configuration();
    memcpy(&video, &payload->video_service, sizeof(struct kern_video));

    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
    {
        kernel_assert(vga_init((struct kern_video_vga*)video.info_structure) == 0);
        goto end;
    }
    case VIDEO_VBE_TEXT:
    {
        kernel_assert(vbe_init((struct kern_video_vbe*)video.info_structure) == 0);
        goto end;
    }
    }

    panic("VIDEO: Invalid video mode!");

end:
    video_clear();
    console_update_video();
}

void video_setmode(uint8_t mode, uint32_t width, uint32_t height)
{
    panic("VIDEO: video_setmode is not implemented.");
}

void video_clear()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
    {
        vga_clear();
        return;
    }
    case VIDEO_VBE_TEXT:
    {
        vbe_clear();
        return;
    }
    }

    panic("VIDEO: Invalid video mode!");
}

int video_text_print_at(uint32_t *data, uint8_t color, size_t offset, size_t length)
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return vga_print_at(data, color, offset, length);
    case VIDEO_VBE_TEXT:
        return vbe_print_at(data, color, offset, length);
    }

    panic("VIDEO: Invalid video mode!");
}

int video_text_set_at(char c, uint8_t color, size_t offset)
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return vga_set_at(c, color, offset);
    case VIDEO_VBE_TEXT:
        return vbe_set_at(c, color, offset);
    }

    panic("VIDEO: Invalid video mode!");
}

int video_width()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return vga_rows();
    case VIDEO_VBE_TEXT:
        return vbe_width();
    }

    panic("VIDEO: Invalid video mode!");
}

int video_height()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return vga_columns();
    case VIDEO_VBE_TEXT:
        return vbe_height();
    }

    panic("VIDEO: Invalid video mode!");
}

int video_text_width()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return vga_rows();
    case VIDEO_VBE_TEXT:
        return vbe_text_width();
    }

    panic("VIDEO: Invalid video mode!");
}

int video_text_height()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return vga_columns();
    case VIDEO_VBE_TEXT:
        return vbe_text_height();
    }

    panic("VIDEO: Invalid video mode!");
}

int video_bpp()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
        return 0;
    case VIDEO_VBE_TEXT:
        return vbe_bpp();
    }

    panic("VIDEO: Invalid video mode!");
}

void video_text_render(uint32_t *buffer, size_t width, size_t height)
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
    {
        vga_render(buffer, width, height);
        return;
    }
    case VIDEO_VBE_TEXT:
    {
        vbe_render_text_buffer(buffer, width, height);
        return;
    }
    }

    panic("VIDEO: Invalid video mode!");
}

void *video_fb()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
    {
        return vga_fb();
    }
    case VIDEO_VBE_TEXT:
    {
        return vbe_fb();
    }
    }

    panic("VIDEO: Invalid video mode!");
}

size_t video_fb_size()
{
    switch (video.mode)
    {
    case VIDEO_VGA_TEXT:
    {
        return vga_fb_size();
    }
    case VIDEO_VBE_TEXT:
    {
        return vbe_fb_size();
    }
    }

    panic("VIDEO: Invalid video mode!");
}