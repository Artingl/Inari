#include <kernel/kernel.h>
#include <kernel/include/C/string.h>
#include <kernel/sys/console/console.h>

#include <drivers/video/video.h>

#include <drivers/video/vbe/vbe_drv.h>

struct kern_video video;

void video_init()
{
    struct kernel_payload const *payload = kernel_configuration();
    memcpy(&video, &payload->video_service, sizeof(struct kern_video));

    switch (video.mode)
    {
    case VIDEO_VBE:
    {
        printk(KERN_DEBUG "VIDEO: Using VBE");
        kernel_assert(vbe_init((struct kern_video_vbe*)video.info_structure) == 0, "VBE init failed");
        goto end;
    }
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");

end:
    video_clear();
}

void video_setmode(uint8_t mode, uint32_t width, uint32_t height)
{
    panic("VIDEO: video_setmode is not implemented.");
}

void video_clear()
{
    switch (video.mode)
    {
    case VIDEO_VBE:
    {
        vbe_clear();
        return;
    }
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");
}

int video_width()
{
    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_width();
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");
}

int video_height()
{
    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_height();
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");
}

int video_bpp()
{
    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_bpp();
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");
}

void *video_fb()
{
    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_fb();
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");
}

size_t video_fb_size()
{
    switch (video.mode)
    {
    case VIDEO_VBE:
        return vbe_fb_size();
    case VIDEO_GOP:
    {
        panic("VIDEO: GOP not implemented");
    }
    }

    panic("VIDEO: Invalid video mode!");
}