#include <sys.h>
#include <io.h>
#include <lib.h>
#include <signals.h>
#include <string.h>

char test[12];
char *addr = (char*)0;

void sigsev_handler(uint32_t signo)
{
    // printf("got seg fault 0x%x!\n", addr);
    addr = &test[0];
    sigreturn(); // dont forget sigreturn!
}

void sigquit_handler(uint32_t signo)
{
    printf("SIGQUIT!\n");

    exit(0);
}

int main(int argc, char *argv[])
{
    // signal_handler(&sigsev_handler, SIGSEGV);
    signal_handler(&sigquit_handler, SIGQUIT);
    pid_t pid;
    get_pid(&pid);
    printf("ipc: working! my pid is %llu\n", pid);    

    // while (1)
    // {
        // printf("testing!\n");
        // addr[0] = 'a';
        // printf("works!\n");
        /* Avoid busy looping */
        // usleep(1000000);
    // }

    /* Lets try to switch video mode */
    if (argc < 3)
    {
        printf("ipc: provide at least 2 params.\n");
        return -1;
    }

    handle_t hndl;
    if (open(&hndl, "/dev/video/char_video0", WRITE) != 0)
    {
        printf("ipc: unable to open vesa device.\n");
        return -1;
    }

    /* Try to set resolution */
    struct
    {
        uint32_t width, height;
        uint8_t bpp, allow_similar;
    } mode = {
        .width = atoi(argv[1]),
        .height = atoi(argv[2]),
        .bpp = 32,
        .allow_similar = 0
    };
    int res = ioctl(hndl, 0, &mode);  // IOCTL_MODE_SWITCH
    printf("ipc: ioctl result %d\n", res);

    /* Try to draw on screen (fill half of it)*/
    uint8_t *buf = (uint8_t*)malloc((mode.width >> 1) + (mode.height >> 1) * 3);
    if (!buf)
    {
        printf("ipc: unable to allocate buffer.\n");
        close(hndl);
        return -1;
    }
    /* Fill buffer with white color */
    // memset((void*)buf, 0xff00ff, (mode.width >> 1) + (mode.height >> 1));
    for (size_t i = 0; i < (mode.width >> 1) + (mode.height >> 1) * 3; i+=3)
        { buf[i + 0] = 0xff; buf[i + 1] = 0x00; buf[i + 2] = 0xff; }

    struct
    {
        uint8_t format;
        uint8_t *buffer;
        uint32_t x, y;
        uint32_t width, height;
    } blit = {
        .format = 0,    // VESA_BLIT_R8G8B8_FORMAT
        .buffer = buf,
        .width = mode.width >> 1,
        .height = mode.height >> 1,
        .x = mode.width >> 2,
        .y = mode.height >> 2
    };
    res = ioctl(hndl, 2, &blit);  // IOCTL_BLIT
    printf("ipc: ioctl result %d\n", res);

    free(buf);
    close(hndl);
}
