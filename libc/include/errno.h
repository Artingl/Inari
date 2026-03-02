// Credit: from linux sources

#ifndef _LINUX_ERRNO_H
#define _LINUX_ERRNO_H

#define ENODEV    1  /* No such device */
#define EPERM     2  /* Operation not permitted */
#define EBUSY     3  /* Device or resource busy */
#define EINVAL    4  /* Invalid argument */
#define ENOMEM    5  /* Out of memory */
#define EINVFS    6  /* Invalid file-system */
#define EIO       7  /* I/O error */
#define ETIMEDOUT 8  /* Connection timed out */
#define ENOENT    9  /* No such file or directory */
#define EBADHNDL  10 /* Bad handle */
#define ENOSYS    11 /* Function not implemented */
#define ESRCH     12 /* No such process */
#define EACCES    13 /* Permission denied */
#define EFAULT    14 /* Bad address */
#define ENOEXEC   15 /* Exec format error */
#define EINTR     16 /* Interrupted */
#define EKILLED   17 /* Killed */


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
};

#include <io.h>
#include <sys.h>
#undef errno
#define errno(err)  (errstr[((err) < 0 ? -(err) : err)] ? errstr[((err) < 0 ? -(err) : err)] : "Invalid error")
#define report_errno(err)   do { if (e < 0) {e*=-1;} printf("%s: %s.\n", get_name(), errno(err)); } while(0)

#endif