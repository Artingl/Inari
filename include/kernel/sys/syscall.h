#ifndef _INARI_SYSCALL_H
#define _INARI_SYSCALL_H

#include <misc/types.h>

#define SYSCALL_EXIT           1
#define SYSCALL_USLEEP         2
#define SYSCALL_DEBUG          3
#define SYSCALL_OPEN           4
#define SYSCALL_CLOSE          5
#define SYSCALL_EXECP          6
#define SYSCALL_READ           7
#define SYSCALL_SEEK           8
#define SYSCALL_TELL           9
#define SYSCALL_SIZE           10
#define SYSCALL_GET_PID        11
#define SYSCALL_SPAWN_THREAD   12
#define SYSCALL_GET_TID        13
#define SYSCALL_KILL_THREAD    14
#define SYSCALL_MOUNT          15
#define SYSCALL_UNMOUNT        16
#define SYSCALL_READDIR        17
#define SYSCALL_WRITE          18
#define SYSCALL_WAITPID        19
#define SYSCALL_EXECPV         20
#define SYSCALL_MEMMAP         21
#define SYSCALL_MEMUNMAP       22
#define SYSCALL_MEMALLOC       23
#define SYSCALL_MEMFREE        24
#define SYSCALL_IOCTL          25
#define SYSCALL_SIGNAL         26
#define SYSCALL_INST_SIG       27 // install signal handler
#define SYSCALL_SIGRETURN      28
#define SYSCALL_REBOOT         29
#define SYSCALL_POWEROFF       30
#define SYSCALL_RMMOD          31
#define SYSCALL_INSMOD         32
#define SYSCALL_LSMOD          33
#define SYSCALL_LSPROC         34
#define SYSCALL_FLUSH          35
#define SYSCALL_UNAME          36
#define SYSCALL_UPTIME         37
#define SYSCALL_IPC_CREATE     38
#define SYSCALL_IPC_FREE       39
#define SYSCALL_IPC_FETCH_NEXT 40
#define SYSCALL_IPC_REPLY      41
#define SYSCALL_IPC_OPEN       42
#define SYSCALL_IPC_CLOSE      43
#define SYSCALL_IPC_SEND       44
#define SYSCALL_IPC_WAIT       45
#define SYSCALL_EXECPVF        46
#define SYSCALL_LSTHRD         47
#define SYSCALL_NET            48

int syscall_handle(uint32_t id, void *param0, void *param1, void *param2, void *param3, void *param4);

#endif
