#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <typedefs.h>

typedef int64_t handle_t;
typedef uint64_t tid_t; // Thread ID
typedef uint64_t pid_t; // Process ID

#define	IO_READ			    0x01
#define	IO_WRITE			0x02
#define	IO_OPEN_EXISTING	0x00
#define	IO_CREATE_NEW		0x04
#define	IO_CREATE_ALWAYS	0x08
#define	IO_OPEN_ALWAYS		0x10
#define	IO_OPEN_APPEND		0x30

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

int syscall(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4);

#endif