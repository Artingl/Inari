#pragma once

#include <kernel/include/C/typedefs.h>

#include <drivers/cpu/cpu.h>
#include <drivers/memory/pmm.h>
#include <drivers/video/video.h>

#define KERN_ERR "0"
#define KERN_WARNING "1"
#define KERN_DEFAULT ""

#define INARI ...

#define KERN_MAX_CORES 256
#define KERN_STACK_SIZE 0x100000

#define printk(message...) printk_wrapper(__LINE__, __FILE__, __FUNCTION__, message)
#define kernel_assert(cond, msg) \
    do { if (!(cond))            \
    panic("%s: in %s at line %d", msg, __FILE__, __LINE__); } while(0)

extern void *_hi_start_marker;

#define KERN_PHYS(addr) ((uintptr_t)(addr) - (uintptr_t) & _hi_start_marker)

// wrapper for log functions
int printk_wrapper(size_t line, const char *file, const char *func, const char *fmt, ...);

void panic(const char *message, ...);

// get the kernel uptime (in MS)
double kernel_uptime();

struct kernel_mmap_entry
{
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed));

#define KERN_MMAP_AVAILABLE 1
#define KERN_MMAP_RESERVED 2

struct kernel_payload
{
    struct page_directory *core_directory;

    const char *bootloader; // name of the bootloader
    const char *cmdline;    // kernel command line arguments

    struct kern_video video_service; // video service to be used by the kernel (VGA, VBE, etc.)

    struct kernel_mmap_entry *mmap; // list of mmaps
    size_t mmap_length;             // amount of mmaps in the list
};

const char *kernel_root_mount_point();
void kernel_parse_cmdline();
void kmain(struct kernel_payload *payload);
void ap_kmain(struct cpu_core *core);

// kernel configuration at startup (provided by the bootloader)
struct kernel_payload const *kernel_configuration();

#define KERN_PAGE_PRESENT (1 << 0)
#define KERN_PAGE_RW (1 << 1)
#define KERN_PAGE_USR (1 << 2)
#define KERN_PAGE_DIRTY (1 << 5)

// CUSTOM BIT
#define KERN_PAGE_USED (1 << 10)

#define KERN_TABLE_PRESENT (1 << 0)

#define PAGE_SIZE 0x1000

// kernel identity (paging)
void kident(void *addr, size_t length, uint32_t flags);
void kunident(void *addr, size_t length);

int kmmap(
    void *virtual,
    void *real,
    size_t length,
    uint32_t flags);

void *vmm_alloc_page(size_t pages);
int vmm_free_pages(
    void *pointer,
    size_t pages);

void *kmalloc(size_t length);
void kfree(void *ptr);
void *krealloc(void *ptr, size_t size);
void *kcalloc(size_t n, size_t size);

int kernel_initialize_gdb();
