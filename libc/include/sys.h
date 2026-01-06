#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <typedefs.h>

typedef int64_t handle_t;

typedef void (*thread_entrypoint_t)();
typedef uint64_t tid_t; // Thread ID
typedef uint64_t pid_t; // Process ID
typedef unsigned int dev_t;

#define	IO_READ			    0x01
#define	IO_WRITE			0x02
#define	IO_OPEN_EXISTING	0x00
#define	IO_CREATE_NEW		0x04
#define	IO_CREATE_ALWAYS	0x08
#define	IO_OPEN_ALWAYS		0x10
#define	IO_OPEN_APPEND		0x30

#define IO_STAT_FILE       (1 << 1)
#define IO_STAT_DIR        (1 << 2)
#define IO_STAT_BLOCK      (1 << 3)

struct fs_node {
    char name[256];
    uint32_t st_mode;           // File type + permissions
    uint64_t size;              // File size (bytes)

    size_t off;                 // Node offset in the directory
};

int exit(int code);
int usleep(size_t t);
int debug(const char *s);
int open(handle_t *hndl, const char *path, int flags);
int close(handle_t hndl);
int execp(pid_t *pid, const char *path);
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

int syscall(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4);

#endif