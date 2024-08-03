#pragma once

#include <kernel/include/C/typedefs.h>
#include <kernel/driver/serial/serial.h>

#define KERN_ERR "0"
#define KERN_WARNING "1"
#define KERN_DEFAULT ""

#define INARI ...

#define KERN_MAX_CORES 256
#define KERN_STACK_SIZE 0x2000

#define KERN_SERIAL_DEBUG SERIAL_COM0

#define printk(message...) printk_wrapper(__LINE__, __FILE__, __FUNCTION__, message)
#define kernel_assert(cond, msg) \
    do { if (!(cond))            \
    panic("%s: in %s at line %d", msg, __FILE__, __LINE__); } while(0)

extern char __kvirtual_start;
#define KERN_PHYS(addr) ((uintptr_t)(addr) - (uintptr_t) & __kvirtual_start)

// wrapper for log functions
int printk_wrapper(size_t line, const char *file, const char *func, const char *fmt, ...);

void panic(const char *message, ...);

// get the kernel time (in MS)
double kernel_time();

struct kernel_mmap_entry
{
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

#define KERN_MMAP_AVAILABLE 1
#define KERN_MMAP_RESERVED 2

const char *kernel_cmdline();

void kparse_cmdline();
void kmain(char *cmdline);

#define KERN_PAGE_PRESENT (1 << 0)
#define KERN_PAGE_RW (1 << 1)
#define KERN_PAGE_USR (1 << 2)
#define KERN_PAGE_DIRTY (1 << 5)

// CUSTOM BIT
#define KERN_PAGE_USED (1 << 10)

#define KERN_TABLE_PRESENT (1 << 0)
#define KERN_TABLE_RW (1 << 1)

#define PAGE_SIZE 0x1000
