#include <sys.h>
#include <io.h>
#include <string.h>
#include <signals.h>

#include "core.h"
#include "input.h"
#include "video.h"
#include "window.h"

static int is_ism_running = 1;

int is_running(void) {
    return is_ism_running;
}

void sig_handler(uint32_t sig) {
    is_ism_running = 0;
    sigreturn();
}

int main(int argc, char *argv[])
{
    signal_handler(&sig_handler, SIGQUIT);
    char *video_device = "/devices/video/char_video0";

    /* Parse arguments */
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
    printf("%s: starting.\n", get_name());

    if (ism_video_init((const char*)video_device) != 0) {
        printf("%s: Video init failed.\n", get_name());
        return -1;
    }
    if (input_init() != 0) {
        printf("%s: Input init failed.\n", get_name());
        return -1;
    }
    if (ism_window_init() != 0) {
        printf("%s: Window init failed.\n", get_name());
        return -1;
    }
    if (ism_init_ipc() != 0) {
        printf("%s: IPC init failed.\n", get_name());
        return -1;
    }

    /* Run main ism loop */
    time_t start = 0, end = 0;
    do {
        uptime(&start);
        ism_window_update();
        ism_window_render();
        ism_video_flush();
        uptime(&end);

        /* Around 60 FPS (1000 / 60) */
        usleep(16000 - ((end - start) > 16000 ? 0 : (end - start)));
    } while(is_running());

    ism_ipc_cleanup();
    ism_window_cleanup();
    input_cleanup();
    ism_video_cleanup();
    return 0;
}
