#ifndef _INARI_IPC_H
#define _INARI_IPC_H

#include <kernel/proc/sched.h>
#include <kernel/proc/proc.h>

#include <misc/types.h>

#define IPC_MSG_QUEUE   128    // Max possible pending message events at once
#define IPC_MAX_NAME_LN 256

typedef int64_t ipc_handle_t;

/* This will create a new thread under using `handler`, which can listen for upcoming events using `ipc_fetch_next`.
   The events will arrive in a queue style, one-by-one. */
int ipc_create(struct process *proc, const char *name, thread_entrypoint_t handler);
int ipc_free(struct process *proc, const char *name);
/* Used by IPC handler thread to read message data. The data buffer is directly mapped to the memory
   from caller process, allowing to send result to the caller using exactly the same memory.
   A message ID 0xFFFFFFFF can be sent, which means connection with handle at `source` PID is being closed. */
int ipc_fetch_next(struct process *proc, struct thread *th, pid_t *source, ipc_handle_t *ipc, uint32_t *message, void **data, size_t *data_sz);
/* Called by the IPC handler when it finishes processing the shared memory.
   This unmaps the memory from the handler's virtual space and wakes the sender. */
int ipc_reply(struct process *proc, struct thread *th, int status);

/* Open/close handle to the IPC handler process */
int ipc_open(struct process *proc, const char *name, ipc_handle_t *ipc);
int ipc_close(struct process *proc, ipc_handle_t ipc);

/* Send data to IPC handler process.
   message - Any value, it will be passed to IPC handler for processing.
   data - Pointer to data that will be mapped to IPC handler process (using data_sz as the size).
   If NULL, nothing will be provided to IPC handler. Must be aligned to 4KB. Return: 0 on success. */
int ipc_send(struct process *proc, ipc_handle_t ipc, uint32_t message, void *data, size_t data_sz);
/* Wait for answer from IPC handler.
   do_sleep - Determines whether this call is blocking/non-blocking
   Return: 0 when IPC handler finishes execution, 1 if still pending, negative value if error. */
int ipc_wait(struct process *proc, ipc_handle_t ipc, uint8_t do_sleep);

/* Used by proc.c to announce death of a thread to properly cleanup IPC endpoints */
void ipc_announce_death(struct process *proc, tid_t th);

/* Used by proc.c to cleanup IPC endpoints for a given process */
void ipc_cleanup(struct process *proc);

#endif
