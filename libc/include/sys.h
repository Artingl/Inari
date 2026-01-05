#ifndef _LIBC_SYS_H
#define _LIBC_SYS_H

#include <typedefs.h>

#define exit(code) syscall(1, (code), 0, 0, 0, 0)

int syscall(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4);

#endif