#ifndef _INARI_IPC_H
#define _INARI_IPC_H

typedef int64_t ipc_handle_t;

#include <kernel/proc/sched.h>

#include <misc/types.h>

/* This will create a new thread under using `handler`, which can listen for upcoming events using `ipc_fetch_next`.
   The events will arrive in a queue style, one-by-one. */
int ipc_create(const char *name, thread_entrypoint_t handler);
int ipc_free(const char *name);
/* Used by IPC handler thread to read message data. The data buffer is directly mapped to the memory
   from caller process, allowing to send result to the caller using exactly the same memory */
int ipc_fetch_next(uint32_t *message, void **data, size_t *data_sz);
/* Called by the IPC handler when it finishes processing the shared memory.
   This unmaps the memory from the handler's virtual space and wakes the sender. */
int ipc_reply(int status);

/* Open/close handle to the IPC handler process */
int ipc_open(const char *name, ipc_handle_t *ipc);
int ipc_close(ipc_handle_t ipc);

/* Send data to IPC handler process.
   message - Any value, it will be passed to IPC handler for processing.
   data - Pointer to data that will be mapped to IPC handler process (using data_sz as the size, max 16KB of data at once).
          If NULL, nothing will be provided to IPC handler. Must be aligned to 4KB.
   Return: 0 on success. */
int ipc_send(ipc_handle_t ipc, uint32_t message, void *data, size_t data_sz);
/* Wait for answer from IPC handler.
   do_sleep - Determines whether this call is blocking/non-blocking
   Return: 0 when IPC handler finishes execution, 1 if still pending, negative value if error. */
int ipc_wait(ipc_handle_t ipc, uint8_t do_sleep);

#endif