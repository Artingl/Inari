// Credit: from linux sources


#ifndef _INARI_ERRNO_H
#define _INARI_ERRNO_H

#define ENODEV     2  /* No such device */
#define EPERM      3  /* Operation not permitted */
#define EBUSY      4  /* Device or resource busy */
#define EINVAL     5  /* Invalid argument */
#define ENOMEM     6  /* Out of memory */
#define EINVFS     7  /* Invalid file-system */
#define EIO        8  /* I/O error */
#define ETIMEDOUT  9  /* Connection timed out */
#define ENOENT     10  /* No such file or directory */
#define EBADHNDL   11 /* Bad handle */
#define ENOSYS     12 /* Function not implemented */
#define ESRCH      13 /* No such process */
#define EACCES     14 /* Permission denied */
#define EFAULT     15 /* Bad address */
#define ENOEXEC    16 /* Exec format error */
#define EINTR      17 /* Interrupted */
#define EKILLED    18 /* Killed */
#define ESUBSYSDIS 19 /* Subsystem disabled */

#define IPCBSY   20 /* IPC is busy */
#define IPCNONE  21 /* IPC endpoint not found */
#define IPCACCES 22 /* IPC access denied */

__attribute__((unused)) static const char *errstr[] = {
    [0] = "OK",
    [ENODEV] = "No such device",
    [EPERM] = "Operation not permitted",
    [EBUSY] = "Device or resource busy",
    [EINVAL] = "Invalid argument",
    [ENOMEM] = "Out of memory",
    [EINVFS] = "Invalid file-system",
    [EIO] = "I/O error",
    [ETIMEDOUT] = "Connection timed out",
    [ENOENT] = "No such file or directory",
    [EBADHNDL] = "Bad handle",
    [ENOSYS] = "Function not implemented",
    [ESRCH] = "No such process",
    [EACCES] = "Permission denied",
    [EFAULT] = "Bad address",
    [ENOEXEC] = "Exec format error",
    [IPCBSY] = "IPC is busy",
    [IPCNONE] = "IPC endpoint not found",
    [IPCACCES] = "IPC access denied",
    [ESUBSYSDIS] = "Subsystem disabled",
};

#include <io.h>
#include <sys.h>
#undef errno
#define errno(err) (errstr[((err) < 0 ? -(err) : err)] ? errstr[((err) < 0 ? -(err) : err)] : "Invalid error")
#define report_errno(err)                                                                                              \
    do {                                                                                                               \
        if (e < 0) {                                                                                                   \
            e *= -1;                                                                                                   \
        }                                                                                                              \
        printf("%s: %s.\n", get_name(), errno(err));                                                                   \
    } while (0)
#endif
