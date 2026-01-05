#ifndef _INARI_SYSCALL_H
#define _INARI_SYSCALL_H

#include <misc/types.h>

#define SYSCALL_EXIT 1


int syscall_handle(
    uint32_t id,
    uint32_t param0,
    uint32_t param1,
    uint32_t param2,
    uint32_t param3,
    uint32_t param4);

#endif