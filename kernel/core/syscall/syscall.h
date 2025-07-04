#pragma once

#include <kernel/libc/typedefs.h>

#define KERN_SYSCALL_READ                  0x00
#define KERN_SYSCALL_WRITE                 0x01
#define KERN_SYSCALL_SLEEP                 0x02
#define KERN_SYSCALL_SCHEDULER_YIELD       0xaf

#define KERN_MAX_SYSCALLS                  0xff

extern uint32_t arch_syscall(uint8_t id, uint32_t param0, uint32_t param1, uint32_t param2);

#define kern_syscall(id, param0, param1, param2) arch_syscall(id, (uint32_t)(param0), (uint32_t)(param1), (uint32_t)(param2))

// id, param0, param1, param2
typedef uint32_t(syscall_handler_t)(uint32_t, uint32_t, uint32_t, uint32_t, void*);

uint32_t kern_syscall_handle(uint8_t id, uint32_t param0, uint32_t param1, uint32_t param2, void *regs_ptr);
void kern_syscall_register(uint8_t id, syscall_handler_t *handler);
