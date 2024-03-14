#include <kernel/kernel.h>
#include <kernel/tests/kernel_tests.h>

#include <drivers/fs/fat/fat32.h>

#include <drivers/video/video.h>
#include <drivers/memory/memory.h>
#include <drivers/memory/vmm.h>
#include <drivers/cpu/cpu.h>
#include <drivers/cpu/amd/svm/svm.h>
#include <drivers/serial/serial.h>
#include <drivers/ps2/ps2.h>

#include <kernel/scheduler/scheduler.h>
#include <kernel/scheduler/thread.h>
#include <kernel/sys/console/console.h>
#include <kernel/sys/devfs/devfs.h>
#include <kernel/sys/disks/disks.h>
#include <kernel/sys/vfs/vfs.h>
#include <kernel/sys/sys.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

// the payload should be in the lower memory so we can use it anywhere we want even without paging
__attribute__((section(".lo_text"))) struct kernel_payload payload;

extern void *_hi_start_marker;
extern void *_hi_end_marker;

void kernel_scheduled(void*);

void kmain(struct kernel_payload *__payload)
{
    const char *mount_point;
    struct devfs_node *block_device;
    struct gendisk *disk;

    memcpy(&payload, __payload, sizeof(struct kernel_payload));

    console_init();
    memory_init();
    memory_info();
    console_enable_heap();
    video_init();

    printk(KERN_INFO "Inari kernel (x86, %s)", payload.bootloader);
    printk(KERN_INFO "Kernel cmdline: %s", payload.cmdline);
    printk(KERN_INFO "Kernel virtual start: %p", &_hi_start_marker);
    printk(KERN_INFO "Kernel virtual end: %p", &_hi_end_marker);

    // Make some small kernel tests
    kernel_make_tests();

    cpu_bsp_init();
    scheduler_init();
    sys_init();

    // initialize drivers
    ps2_init();

    // parse cmdline to initialize the kernel itself
    kernel_parse_cmdline();

#if 0
    // mount the root
    mount_point = kernel_root_mount_point();
    if (mount_point == NULL)
        panic("kernel: unspecified mount point.");

    block_device = devfs_get_node(mount_point);
    if (block_device == NULL)
        panic("kernel: block device '%s' not found.", mount_point);

    disk = alloc_disk(block_device);
    kernel_assert(vfs_mount_root(disk) == VFS_SUCCESS, "vfs init failed");

    struct vfs_directory *dir = vfs_opendir("/");
    struct vfs_entry *entry;

    if (dir)
    {
        while ((entry = vfs_readdir(dir)))
        {
            printk("%s", entry->entry_path);
        }

        vfs_closedir(dir);
    }

    printk("vfs: total mount points %d", vfs_mount_points());
#endif

    memory_info();
    printk("HELLO!\n");

    // thread_t ths[128];
    // for (size_t i = 0; i < 128; i++) {
    //     thread_init(&ths[i], NULL, &kernel_scheduled);
    //     // thread_start(&ths[i]);
    // }

    // // Run scheduler for the core
    // scheduler_enter(cpu_current_core());

    while (1) {}
}

void ap_kmain(struct cpu_core *core)
{
    // scheduler_enter(core);
    printk("HELLO!\n");
    while (1) {}
}

struct kernel_payload const *kernel_configuration()
{
    return &payload;
}

extern double cpu_timer_ticks;

double kernel_uptime()
{
    return cpu_timer_ticks / 1000.0f; // / cpu_timer_freq();
}

volatile void kernel_scheduled(void*)
{
    // for (volatile size_t i = 0; i < 10; i++)
    //     printk("%d", i);

    thread_t *th = thread_self();

    for (volatile size_t i = 0; i < 32; i++)
    {}
        // printk("Hello from thread %d!", th->tid);
    printk("done %d", th->tid);

    // for (volatile size_t i = 0; i < 0xffffff;i++);
    // printk("done %d", th->tid);
    // machine_reboot();
    
    // thread_kill(th, 0);
    volatile int f = 0;
    while (1) {
        f ++;
    }
}
