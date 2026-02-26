#include <sys.h>
#include <io.h>
#include <lib.h>
#include <signals.h>
#include <string.h>
#include <errno.h>

#include <kernel/subsys/video.h>
#include <kernel/subsys/hid.h>

int main(int argc, char *argv[])
{
    struct video_mode_info info = {0};
    struct video_blit mouse_blit;
    struct mouse_event mouse;
    handle_t video_hndl, mouse_hndl;
    char *video_device = "/dev/video/char_video0";
    int res;
    int mouse_x = 0, mouse_y = 0, rel_x = 0, rel_y = 0;

    if (argc >= 2 && strcmp(argv[1], "-d") == 0)
    {
        if (argc >= 3)
            video_device = argv[2];
        else
        {
            printf("usage: %s [-d video_device]\n", argv[0]);
            return -1;
        }
    }

    /* Open video device */
    if ((res = open(&video_hndl, video_device, WRITE)) != 0)
    {
        printf("%s: unable to open video device %s: %s\n", argv[0], video_device, errstr[-res] ? errstr[-res] : "Invalid error");
        return res;
    }

    /* Open mouse device */
    if ((res = open(&mouse_hndl, "/dev/input/char_mouse0", WRITE)) != 0)
    {
        close(video_hndl);
        printf("%s: unable to open mouse device: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
        return res;
    }

    /* Fetch current video mode */
    res = ioctl(video_hndl, VIDEO_IOCTL_INFO, &info);
    if (res != 0)
    {
        printf("%s: ioctl result: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
        close(mouse_hndl);
        close(video_hndl);
        return res;
    }

    printf("%s: current mode: %dx%d_%d\n", argv[0], info.width, info.height, info.bpp);

    /* Allocate memory for mouse blit */
    uint8_t *buf0 = (uint8_t*)malloc(20 * 20 * 3);
    uint8_t *buf1 = (uint8_t*)malloc(20 * 20 * 3);
    if (!buf1 || !buf0)
    {
        printf("%s: unable to allocate buffer.\n", argv[0]);
        close(video_hndl);
        close(mouse_hndl);
        if (buf0) free(buf0);
        if (buf1) free(buf1);
        return -1;
    }

    mouse_blit.format = VIDEO_R8G8B8_FORMAT;
    mouse_blit.buffer = buf0;
    mouse_blit.width = 20;
    mouse_blit.height = 20;
    mouse_blit.x = 0;
    mouse_blit.y = 0;

    /* Fill mouse buffer with some static color */
    memset((void*)buf0, 0xff, 20 * 20 * 3);
    memset((void*)buf1, 0x0, 20 * 20 * 3);

    /* Draw the mouse */
    do {
        if (read(mouse_hndl, (void*)&mouse, sizeof(struct mouse_event), NULL) != 0)
        {
            printf("%s: unable to read mouse: %s.", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
            close(video_hndl);
            close(mouse_hndl);
            if (buf0) free(buf0);
            if (buf1) free(buf1);
            return -1;
        }

        if (mouse.buttons[HID_MOUSE_BTN3])
        {
            printf("%s: mouse click!\n", argv[0]);
            break;
        }

        if (rel_x != mouse.rel_x || rel_y != mouse.rel_y)
        {
            mouse_x += mouse.rel_x;// * 10;
            mouse_y += mouse.rel_y;// * 10;

            if (mouse_x < 0) mouse_x = 0;
            if (mouse_y < 0) mouse_y = 0;
            if (mouse_x > info.width - 20) mouse_x = info.width - 20;
            if (mouse_y > info.height - 20) mouse_y = info.height - 20;

            /* Clear old mouse position */
            if (!mouse.buttons[HID_MOUSE_BTN1])
            {
                mouse_blit.buffer = buf1;
                res = ioctl(video_hndl, VIDEO_IOCTL_BLIT, &mouse_blit);
            }

            /* Redraw mouse */
            mouse_blit.buffer = buf0;
            mouse_blit.x = mouse_x;
            mouse_blit.y = mouse_y;
            res = ioctl(video_hndl, VIDEO_IOCTL_BLIT, &mouse_blit);
            if (res != 0)
            {
                printf("%s: ioctl error: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
                break;
            }
        }

        rel_x = mouse.rel_x;
        rel_y = mouse.rel_y;
    } while (1);

    if (buf0) free(buf0);
    if (buf1) free(buf1);
    close(mouse_hndl);
    close(video_hndl);
    return 0;
}
