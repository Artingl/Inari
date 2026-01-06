#ifndef _INARI_SYSCALL_H
#define _INARI_SYSCALL_H

#include <misc/types.h>

#define SYSCALL_EXIT             1
#define SYSCALL_USLEEP           2
#define SYSCALL_DEBUG            3
#define SYSCALL_OPEN             4
#define SYSCALL_CLOSE            5
#define SYSCALL_EXECP            6
#define SYSCALL_READ             7
#define SYSCALL_SEEK             8
#define SYSCALL_TELL             9
#define SYSCALL_SIZE            10
#define SYSCALL_GET_PID         11
#define SYSCALL_SPAWN_THREAD    12
#define SYSCALL_GET_TID         13
#define SYSCALL_KILL_THREAD     14
#define SYSCALL_MOUNT           15
#define SYSCALL_UNMOUNT         16
#define SYSCALL_READDIR         17


int syscall_handle(
    uint32_t id,
    void *param0,
    void *param1,
    void *param2,
    void *param3,
    void *param4);

#endif