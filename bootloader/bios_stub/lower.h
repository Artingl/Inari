#pragma once

#include <drivers/memory/vmm.h>

#include <kernel/multiboot.h>
#include <kernel/include/C/typedefs.h>

#define LKERN __attribute__((section(".lo_text")))

#define bl_panic(msg)                          \
    {                                          \
        lower_vga_print((char*)msg);                        \
        lower_vga_print((char*)MESSAGES_POOL[MSG_DIR_HLT]); \
        __asm__ volatile("hlt");               \
    }
#define bl_debug(msg...) __lower_vga_printf_wrapper((char*)msg);

extern char *LO_MESSAGE_DEBUG;
extern char *LO_MESSAGE_FILLING;
extern char *LO_MESSAGE_DONE;
extern char *_VGA_PREFIX;
extern char *LO_MESSAGE_IDENTIFY;
extern char *LO_MESSAGE_NO_MMAPS;
extern char *LO_MESSAGE_DIR_DEBUG;
extern char *LO_MESSAGE_HALT;
extern char *LO_MESSAGE_PASS_CONTROL;

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
static volatile __attribute__((section(".lo_rodata")))
const char *MESSAGES_POOL[] = {
    [MSG_PREFIX] = (char *)&_VGA_PREFIX,
    [MSG_DEBUG] = (char *)&LO_MESSAGE_DEBUG,
    [MSG_FILLING] = (char *)&LO_MESSAGE_FILLING,
    [MSG_DONE] = (char *)&LO_MESSAGE_DONE,
    [MSG_IDENTIFY] = (char *)&LO_MESSAGE_IDENTIFY,
    [MSG_NO_MMAPS] = (char *)&LO_MESSAGE_NO_MMAPS,
    [MSG_DIR_DEBUG] = (char *)&LO_MESSAGE_DIR_DEBUG,
    [MSG_DIR_HLT] = (char *)&LO_MESSAGE_HALT,
    [MSG_PASS_CONTROL] = (char*)&LO_MESSAGE_PASS_CONTROL,
};

struct early_alloc_info
{
    uintptr_t heap_top;
    uintptr_t heap;
    uintptr_t heap_end;
};

LKERN void lower_vga_init();
LKERN void __lower_vga_printf_wrapper(char *fmt, ...);
LKERN void paging_mmap(void *offset, void *ptr, size_t size, uint32_t flags);
LKERN void lower_vga_printc(char c);
LKERN struct page_table *paging_get_table(unsigned long table);
LKERN void early_alloc_setup(multiboot_info_t *multiboot);
LKERN void paging_ident(void *ptr, size_t size, uint32_t flags);
LKERN void switch_page(struct page_directory *dir);
LKERN void *early_alloc(size_t i);
LKERN void lower_vga_add(char *addr);
LKERN void lower_vga_print(char *addr);
LKERN struct early_alloc_info early_alloc_info();
