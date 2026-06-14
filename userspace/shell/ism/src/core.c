#include <io.h>
#include <sys.h>

#include "core.h"
#include "window.h"

static void ipc_handler_window() {
    pid_t source;
    handle_t ipc;
    uint32_t message;
    void *data;
    size_t data_size;

    do {
        int reply = -1;
        int fetch = ipc_fetch_next(&source, &ipc, &message, &data, &data_size);

        if (fetch == 0) {
            reply = window_handle_event(source, message, data, data_size);
        }
        else
            printf("ipc: bogus IPC message");

        ipc_reply(reply);
    } while (is_running());
}

int ism_init_ipc(void) {
    return ipc_create("ism.window", &ipc_handler_window);
}

int ism_ipc_cleanup(void) {
    return ipc_free("ism.window");
}
