#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <types.h>

typedef int64_t handle_t;
#ifdef time_t
#undef time_t
#endif
typedef int64_t time_t;

typedef void (*thread_entrypoint_t)();
typedef void (*proc_signal_t)(uint32_t);
typedef uint64_t tid_t; // Thread ID
typedef uint64_t pid_t; // Process ID
typedef unsigned int dev_t;

#define MEM_RW      (1 << 0)
#define MEM_USR     (1 << 1)
#define MEM_PRESENT (1 << 2)

#define READ          0x01
#define WRITE         0x02
#define OPEN_EXISTING 0x00
#define CREATE_NEW    0x04
#define CREATE_ALWAYS 0x08
#define OPEN_ALWAYS   0x10
#define OPEN_APPEND   0x30

#define STAT_FILE  (1 << 0)
#define STAT_DIR   (1 << 1)
#define STAT_BLOCK (1 << 2)
#define STAT_CHAR  (1 << 3)


#define EXEC_FLAG_CP_OPTIONS (1 << 0) // Copies options from the caller process

union p_option_value {
    char value[256];
    uint32_t u32;
    uint64_t u64;
    size_t sz;
} __attribute__((packed));

struct p_option {
    char name[32];
    union p_option_value value;
    struct p_option *next;
    struct p_option *prev;
} __attribute__((packed));

struct fs_node {
    char name[256];
    uint32_t st_mode; // File type + permissions
    uint64_t size;    // File size (bytes)

    uint32_t off; // Node offset in the directory
} __attribute__((packed));

struct utsname {
    char sysname[16];  /* Operating system name */
    char nodename[16]; /* Name within communications network
                          to which the node is attached, if any */
    char release[16];  /* Operating system release
                          (e.g., "2.6.28") */
    char version[16];  /* Operating system version */
    char machine[16];  /* Hardware type identifier */
} __attribute__((packed));

int p_option_get(const char *name, union p_option_value *result);
int p_option_set(const char *name, union p_option_value value);

int exit(int code);
int usleep(size_t t);
int open(handle_t *hndl, const char *path, int flags);
int close(handle_t hndl);
int execp(pid_t *pid, const char *path);
int execpv(pid_t *pid, const char *path, int argc, char **argv);
int execpvf(pid_t *pid, const char *path, int flags, int argc, char **argv);
int read(handle_t hndl, void *buf, size_t len, size_t *rlen);
int seek(handle_t hndl, size_t offset);
int tell(handle_t hndl, size_t *offset);
int size(handle_t hndl, size_t *size);
int get_pid(pid_t *pid);
int spawn_thread(tid_t *tid, pid_t pid, thread_entrypoint_t entrypoint);
int get_tid(tid_t *tid);
int kill_thread(tid_t tid);
int mount(dev_t dev, const char *path);
int unmount(const char *path);
int readdir(const char *path, struct fs_node *node);
int write(handle_t hndl, const void *buf, size_t sz);
int waitpid(pid_t pid);
void *memalloc(size_t npages, uint32_t flags); // todo: this syscall currently ignores flags value
void memfree(void *vbase, size_t npages);
void *memmap(void *vbase, void *pbase, size_t len, uint32_t flags);
void memunmap(void *vbase, size_t len);
int ioctl(handle_t hndl, unsigned long req, void *arg);
int signal(pid_t pid, uint32_t signo);
int signal_handler(proc_signal_t handler, uint32_t signo);
void sigreturn();
void reboot(void);
void poweroff(void);
int rmmod(const char *name);
int insmod(const char *name);
int lsmod(int idx, char *name, uintptr_t *ptr, uint32_t *flags);
int lsproc(int idx, char *name, pid_t *pid, double *usg);
int flush_hndl(handle_t hndl);
int uname(struct utsname *buf);
int time(time_t *t);

/* This will create a new thread under using `handler`, which can listen for upcoming events using `ipc_fetch_next`.
   The events will arrive in a queue style, one-by-one. */
int ipc_create(const char *name, thread_entrypoint_t handler);
int ipc_free(const char *name);
/* Used by IPC handler thread to read message data. The data buffer is directly mapped to the memory
   from caller process, allowing to send result to the caller using exactly the same memory.
   A message with 0xFFFFFFFF can be sent, which means connection with handle at `source` PID is being closed. */
int ipc_fetch_next(pid_t *source, handle_t *ipc, uint32_t *message, void **data, size_t *data_sz);
/* Called by the IPC handler when it finishes processing the shared memory.
   This unmaps the memory from the handler's virtual space and wakes the sender. */
int ipc_reply(int status);

/* Open/close handle to the IPC handler process */
int ipc_open(const char *name, handle_t *ipc);
int ipc_close(handle_t ipc);

/* Send data to IPC handler process.
   message - Any value, it will be passed to IPC handler for processing.
   data - Pointer to data that will be mapped to IPC handler process (using data_sz as the size).
   If NULL, nothing will be provided to IPC handler. Must be aligned to 4KB. Return: 0 on success. */
int ipc_send(handle_t ipc, uint32_t message, void *data, size_t data_sz);
/* Wait for answer from IPC handler.
   do_sleep - Determines whether this call is blocking/non-blocking
   Return: 0 when IPC handler finishes execution, 1 if still pending, negative value if error. */
int ipc_wait(handle_t ipc, uint8_t do_sleep);

const char *get_name(void);
const char **get_argv(void);
int get_argc(void);

int syscall(uint32_t id, uint32_t param0, uint32_t param1, uint32_t param2, uint32_t param3, uint32_t param4);

#endif
