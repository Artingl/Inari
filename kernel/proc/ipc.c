#include <kernel/inari.h>
#include <kernel/proc/ipc.h>

int ipc_create(const char *name, thread_entrypoint_t handler) { return 0; }

int ipc_free(const char *name) { return 0; }

int ipc_fetch_next(pid_t *source, uint32_t *message, void **data, size_t *data_sz) { return 0; }

int ipc_reply(int status) { return 0; }

int ipc_open(const char *name, ipc_handle_t *ipc) { return 0; }
int ipc_close(ipc_handle_t ipc) { return 0; }

int ipc_send(ipc_handle_t ipc, uint32_t message, void *data, size_t data_sz) { return 0; }

int ipc_wait(ipc_handle_t ipc, uint8_t do_sleep) { return 0; }
