#ifndef _INARI_H
#define _INARI_H

#include <stddef.h>
#include <stdint.h>

#include <kernel/fault/panic.h>
#include <kernel/printk.h>

#define ALIGN(val, alg) (((val) + (alg)-1) / (alg) * (alg))

#define PAGE_SIZE 0x1000           // Size of a single page in memory
#define VIRTUAL_ADDR 0xC0000000    // The physical memory address where kernel resides

#define _lo_data __attribute__((used, section("._lo_kern_data"), aligned(PAGE_SIZE)))
#define _lo_text __attribute__((used, section("._lo_kern_text")))

#define fallthrough __attribute__ ((fallthrough))

typedef struct
{
    uint32_t bootloader_magic;
    void *bootloader_info;
    const char *cmdline;
} bootinfo_t;

typedef struct reserved_memory {
    uintptr_t start;
    uintptr_t end;
} reserved_memory_t;

// Performs all early initialization: earlycon device, etc.
extern void kearly_init(bootinfo_t bootinfo);

// The main kernel entrypoint after the hardware was initialized
extern void kmain(void);
extern bootinfo_t get_boot_info();


#define ARG_MAX_LEN 64

extern int parse_cmdline_argument(const char *key, char *result);
extern const char *get_cmdline();

#endif

