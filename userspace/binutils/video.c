#include <sys.h>
#include <io.h>
#include <lib.h>
#include <signals.h>
#include <string.h>
#include <errno.h>

#include <kernel/subsys/video.h>

int main(int argc, char *argv[])
{
    struct video_mode_info info = {0};
    handle_t hndl;
    char *video_device = "/devices/video/char_video0";
    int res, argv_off = 0;

    if (argc < 2)
    {
        printf("usage: %s [-sli] [-d video_device]\n", argv[0]);
        return -1;
    }

    if (strcmp(argv[1], "-s") == 0) argv_off++;

    if (argc >= (3 + argv_off) && strcmp(argv[2 + argv_off], "-d") == 0)
    {
        if (argc >= (4 + argv_off))
            video_device = argv[3 + argv_off];
        else
        {
            printf("usage: %s [-d video_device]\n", argv[0]);
            return -1;
        }
    }

    if ((res = open(&hndl, video_device, WRITE)) != 0)
    {
        printf("%s: unable to open device %s\n", argv[0], video_device);
        return res;
    }

    if (strcmp(argv[1], "-l") == 0)
    {
        printf("%s: listing all modes:\n", argv[0]);
        while (ioctl(hndl, VIDEO_IOCTL_MODE_FIND_NEXT, &info) > 0)
            printf("  0x%4x: %dx%d_%d%s\n", info.mode_id, info.width, info.height, info.bpp, info.is_default ? " (default)" : "");
    }
    else if (strcmp(argv[1], "-i") == 0)
    {
        res = ioctl(hndl, VIDEO_IOCTL_INFO, &info);
        if (res != 0)
        {
            printf("%s: ioctl result: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
            close(hndl);
            return res;
        }

        printf("%s: current mode: %dx%d_%d\n", argv[0], info.width, info.height, info.bpp);
    }
    else if (strcmp(argv[1], "-s") == 0)
    {
        if (argc < 5)
        {
            printf("usage: %s [-s width height bpp]\n", argv[0]);
            close(hndl);
            return -1;
        }

        info.width = atoi(argv[2]);
        info.height = atoi(argv[3]);
        info.bpp = atoi(argv[4]);

        printf("%s: setting mode %dx%d_%d\n", argv[0], info.width, info.height, info.bpp);

        res = ioctl(hndl, VIDEO_IOCTL_MODE_SWITCH, &info);
        if (res != 0)
        {
            printf("%s: ioctl result: %s.\n", argv[0], errstr[-res] ? errstr[-res] : "Invalid error");
            close(hndl);
            return res;
        }

        printf("%s: current mode: %dx%d_%d\n", argv[0], info.width, info.height, info.bpp);
    }
    else {
        printf("usage: %s [-sli] [-d video_device]\n", argv[0]);
        close(hndl);
        return -1;
    }

    /* Try to set resolution */
    // struct
    // {
    //     uint32_t width, height;
    //     uint8_t bpp, allow_similar;
    // } mode = {
    //     .width = atoi(argv[1]),
    //     .height = atoi(argv[2]),
    //     .bpp = 32,
    //     .allow_similar = 0
    // };
    // int res = ioctl(hndl, 0, &mode);  // IOCTL_MODE_SWITCH
    // printf("%s: ioctl result %d\n", argv[0], res);

    // /* Try to draw on screen (fill half of it)*/
    // uint8_t *buf = (uint8_t*)malloc((mode.width >> 1) + (mode.height >> 1) * 3);
    // if (!buf)
    // {
    //     printf("%s: unable to allocate buffer.\n", argv[0]);
    //     close(hndl);
    //     return -1;
    // }
    // /* Fill buffer with white color */
    // // memset((void*)buf, 0xff00ff, (mode.width >> 1) + (mode.height >> 1));
    // for (size_t i = 0; i < (mode.width >> 1) + (mode.height >> 1) * 3; i+=3)
    //     { buf[i + 0] = 0xff; buf[i + 1] = 0x00; buf[i + 2] = 0xff; }

    // struct
    // {
    //     uint8_t format;
    //     uint8_t *buffer;
    //     uint32_t x, y;
    //     uint32_t width, height;
    // } blit = {
    //     .format = 0,    // VESA_BLIT_R8G8B8_FORMAT
    //     .buffer = buf,
    //     .width = mode.width >> 1,
    //     .height = mode.height >> 1,
    //     .x = mode.width >> 2,
    //     .y = mode.height >> 2
    // };
    // res = ioctl(hndl, 2, &blit);  // IOCTL_BLIT
    // printf("%s: ioctl result %d\n", argv[0], res);

    // free(buf);

    close(hndl);
    return 0;
}
