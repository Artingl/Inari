#pragma once

#include <drivers/memory/vmm.h>

#include <kernel/multiboot.h>
#include <kernel/include/C/typedefs.h>

#define BOOTL __attribute__((section(".bootloader.text")))

#define PAGE_SIZE 0x1000

#define PAGE_PRESENT (1 << 0)
#define PAGE_RW (1 << 1)
#define PAGE_USR (1 << 2)

#define bl_panic(msg)                          \
    {                                          \
        lower_vga_print(msg);                        \
        lower_vga_print(MESSAGES_POOL[MSG_DIR_HLT]); \
        __asm__ volatile("hlt");               \
    }
#define bl_debug(msg...) __lower_vga_printf_wrapper(msg);

extern char *_LOWER_MESSAGE_DEBUG;
extern char *_LOWER_MESSAGE_FILLING;
extern char *_LOWER_MESSAGE_DONE;
extern char *_VGA_PREFIX;
extern char *_LOWER_MESSAGE_IDENTIFY;
extern char *_LOWER_MESSAGE_NO_MMAPS;
extern char *_LOWER_MESSAGE_DIR_DEBUG;
extern char *_LOWER_MESSAGE_HALT;
extern char *_LOWER_MESSAGE_PASS_CONTROL;

enum
{
    MSG_PREFIX = 0,
    MSG_DEBUG = 1,
    MSG_FILLING = 2,
    MSG_DONE = 3,
    MSG_IDENTIFY = 4,
    MSG_NO_MMAPS = 5,
    MSG_DIR_DEBUG = 6,
    MSG_DIR_HLT = 7,
    MSG_PASS_CONTROL = 8,
};

/* We do need to keep all messages in a specific pool, which contains the real address to them (stored in an assembly code.)
 * Since for the lower bootloader we need to link sections manually, I do not know how to change the section where gcc stores strings and stuff.
 */
static volatile __attribute__((section(".bootloader.rodata")))
const char *MESSAGES_POOL[] = {
    [MSG_PREFIX] = (char *)&_VGA_PREFIX,
    [MSG_DEBUG] = (char *)&_LOWER_MESSAGE_DEBUG,
    [MSG_FILLING] = (char *)&_LOWER_MESSAGE_FILLING,
    [MSG_DONE] = (char *)&_LOWER_MESSAGE_DONE,
    [MSG_IDENTIFY] = (char *)&_LOWER_MESSAGE_IDENTIFY,
    [MSG_NO_MMAPS] = (char *)&_LOWER_MESSAGE_NO_MMAPS,
    [MSG_DIR_DEBUG] = (char *)&_LOWER_MESSAGE_DIR_DEBUG,
    [MSG_DIR_HLT] = (char *)&_LOWER_MESSAGE_HALT,
    [MSG_PASS_CONTROL] = (char*)&_LOWER_MESSAGE_PASS_CONTROL,
};

struct early_alloc_info
{
    uintptr_t heap_top;
    uintptr_t heap;
    uintptr_t heap_end;
};

BOOTL void lower_vga_init();
BOOTL void __lower_vga_printf_wrapper(char *fmt, ...);
BOOTL void paging_mmap(void *offset, void *ptr, size_t size, uint32_t flags);
BOOTL void lower_vga_printc(char c);
BOOTL struct page_table *paging_get_table(unsigned long table);
BOOTL void early_alloc_setup(multiboot_info_t *multiboot);
BOOTL void paging_ident(void *ptr, size_t size, uint32_t flags);
BOOTL void switch_page(struct page_directory *dir);
BOOTL void *early_alloc(size_t i);
BOOTL void lower_vga_add(char *addr);
BOOTL void lower_vga_print(char *addr);
BOOTL struct early_alloc_info early_alloc_info();
