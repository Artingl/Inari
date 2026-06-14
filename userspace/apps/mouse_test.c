#include <sys.h>
#include <io.h>
#include <lib.h>
#include <signals.h>
#include <string.h>
#include <errno.h>

#include <kernel/subsys/video.h>
#include <kernel/subsys/hid.h>

#define MOUSE_WIDTH  16
#define MOUSE_HEIGHT 16

#define T 0
#define B 1
#define W 2

const uint8_t cursor_color_bitmap[16][16] = {
    { B, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, B, T, T, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, B, T, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, B, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, B, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, B, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, B, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, W, B, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, W, W, B, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, W, W, W, B, T, T, T, T, T, T },
    { B, W, W, W, W, W, B, B, B, B, T, T, T, T, T, T },
    { B, W, W, B, W, W, B, T, T, T, T, T, T, T, T, T },
    { B, W, B, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { B, B, T, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { T, T, T, T, T, B, W, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, B, T, T, T, T, T, T, T, T }
};

handle_t video_hndl = 0;

void sigquit_handler(uint32_t signal) {
    /* Disable video and return to text mode */
    if (video_hndl) {
        ioctl(video_hndl, VIDEO_IOCTL_DISABLE, NULL);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    signal_handler(&sigquit_handler, SIGQUIT);

    int exit_code = 0;
    struct video_mode_info info = {0};
    struct video_blit blit;
    struct mouse_event mouse;
    handle_t mouse_hndl;
    uint8_t *buf0 = NULL;
    uint8_t *buf1 = NULL;
    uint8_t *fullscreen = NULL;
    char *video_device = "/devices/video/char_video0";
    size_t i, j;
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
    if ((res = open(&mouse_hndl, "/devices/input/char_mouse0", WRITE)) != 0)
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

    /* Fill the whole screen with some color */
    fullscreen = (uint8_t*)malloc(info.width * info.height * 3);
    if (!fullscreen)
    {
        printf("%s: unable to allocate buffer.\n", argv[0]);
        exit_code = -1;
        goto end;
    }

    memset(fullscreen, 0x22, info.width * info.height * 3);
    // for (i = 0; i < info.width * info.height * 3; i+=3)
    //     { fullscreen[i + 0] = 0x }

    blit.buffer = fullscreen;
    blit.width = info.width;
    blit.height = info.height;
    blit.x = 0;
    blit.y = 0;
    res = ioctl(video_hndl, VIDEO_IOCTL_BLIT, &blit);
    if (res != 0)
    {
        printf("%s: ioctl error: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
        exit_code = -1;
        goto end;
    }
    free(fullscreen);

    /* Allocate memory for mouse blit */
    buf0 = (uint8_t*)malloc(MOUSE_WIDTH * MOUSE_HEIGHT * 3);
    buf1 = (uint8_t*)malloc(MOUSE_WIDTH * MOUSE_HEIGHT * 3);
    if (!buf1 || !buf0)
    {
        printf("%s: unable to allocate buffer.\n", argv[0]);
        exit_code = -1;
        goto end;
    }

    blit.format = VIDEO_R8G8B8_FORMAT;
    blit.buffer = buf0;
    blit.width = MOUSE_WIDTH;
    blit.height = MOUSE_HEIGHT;
    blit.x = 0;
    blit.y = 0;

    for (i = 0; i < MOUSE_WIDTH; i++)
        for (j = 0; j < MOUSE_HEIGHT; j++)
        {
            uint8_t color = cursor_color_bitmap[j][i];
            color = color == T ? 0x22 : color == B ? 0x00 : 0xff;

            buf0[j * 3 * MOUSE_WIDTH + i * 3 + 0] = color;
            buf0[j * 3 * MOUSE_WIDTH + i * 3 + 1] = color;
            buf0[j * 3 * MOUSE_WIDTH + i * 3 + 2] = color;
        }

    memset((void*)buf1, 0x22, MOUSE_WIDTH * MOUSE_HEIGHT * 3);  // buffer to clear screen

    /* Draw the mouse */
    do {
        if (read(mouse_hndl, (void*)&mouse, sizeof(struct mouse_event), NULL) != 0)
        {
            printf("%s: unable to read mouse: %s.", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
            exit_code = -1;
            goto end;
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
            if (mouse_x > (int)info.width - 1) mouse_x = (int)info.width - 1;
            if (mouse_y > (int)info.height - 1) mouse_y = (int)info.height - 1;

            /* Clear old mouse position */
            blit.width = MOUSE_WIDTH;
            blit.height = MOUSE_HEIGHT;
            blit.buffer = buf1;
            ioctl(video_hndl, VIDEO_IOCTL_BLIT, &blit);

            /* Draw with mouse */
            if (mouse.buttons[HID_MOUSE_BTN1])
            {
                uint8_t single_pixel[3] = {0xff, 0xff, 0xff};
                blit.buffer = (uint8_t*)&single_pixel;
                blit.width = 1;
                blit.height = 1;
                ioctl(video_hndl, VIDEO_IOCTL_BLIT, &blit);
            }

            /* Redraw mouse */
            blit.buffer = buf0;
            blit.width = MOUSE_WIDTH;
            blit.height = MOUSE_HEIGHT;
            blit.x = mouse_x;
            blit.y = mouse_y;
            res = ioctl(video_hndl, VIDEO_IOCTL_BLIT, &blit);
            if (res != 0)
            {
                printf("%s: ioctl error: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
                break;
            }
        }

        rel_x = mouse.rel_x;
        rel_y = mouse.rel_y;
    } while (1);

end:
    /* Disable video and return to text mode */
    ioctl(video_hndl, VIDEO_IOCTL_DISABLE, NULL);

    if (buf0) free(buf0);
    if (buf1) free(buf1);
    close(mouse_hndl);
    close(video_hndl);
    return exit_code;
}
