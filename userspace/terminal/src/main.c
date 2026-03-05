#include <sys.h>
#include <io.h>

#include <ism.h>

handle_t ism_ipc;

int main(void) {
    if (ipc_open("ism.window", &ism_ipc) != 0) {
        printf("%s: Unable to open IPC with ISM.\n", get_name());
        return -1;
    }

    /* Create a window */
    wind_t wind;
    struct ism_create_window  __attribute__ ((aligned (0x1000))) window = {
        .name = "Terminal",
        .x = 40,
        .y = 40,
        .width = 640,
        .height = 400,
        .flags = ISM_WINDOW_FLAG_BORDER | ISM_WINDOW_FLAG_FRAME,
        .parent = ISM_WINDOW_ROOT
    };
    ipc_send(ism_ipc, ISM_IPC_CREATE_WINDOW, &window, sizeof(window));
    if ((wind = ipc_wait(ism_ipc, 1)) < 0) {
        printf("%s: Unable to create window.\n", get_name());
        return -1;
    }

    do {
    } while (1);
    // printf("terminal: HELLO!\n");
    // usleep(5000000);
    // printf("terminal: BYE!\n");
    return 0;
}
