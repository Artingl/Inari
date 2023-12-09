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

void kernel_impl();
void __setup_virtualization();

void kmain(struct kernel_payload *__payload)
{
    const char *mount_point;
    struct devfs_node *block_device;
    struct gendisk *disk;

    memcpy(&payload, __payload, sizeof(struct kernel_payload));

    kernel_impl();
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
    sys_init();

    // initialize drivers
    ps2_init();

    // kernel_assert(devfs_init() == DEVFS_SUCCESS, "devfs init failed");

    // parse cmdline to initialize the kernel itself
    // kernel_parse_cmdline();

    // mount the root
    // mount_point = kernel_root_mount_point();
    // if (mount_point == NULL)
    //     panic("kernel: unspecified mount point.");

    // block_device = devfs_get_node(mount_point);
    // if (block_device == NULL)
    //     panic("kernel: block device '%s' not found.", mount_point);

    // disk = alloc_disk(block_device);
    // kernel_assert(vfs_mount_root(disk) == VFS_SUCCESS, "vfs init failed");

    // struct vfs_directory *dir = vfs_opendir("/");
    // struct vfs_entry *entry;

    // if (dir)
    // {
    //     while ((entry = vfs_readdir(dir)))
    //     {
    //         printk("%s", entry->entry_path);
    //     }

    //     vfs_closedir(dir);
    // }

    // printk("vfs: total mount points %d", vfs_mount_points());

    memory_info();

    int seed = 0;
    for (size_t x = 0; x < 50 && x < 800; x++)
        for (size_t y = 0; y < 50 && y < 600; y++)
        {
            seed = seed * 1664525 + 1013904223;
            *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4)) = seed >> 24;
            seed = seed * 1664525 + 1013904223;
            *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4) + 1) = seed >> 24;
            seed = seed * 1664525 + 1013904223;
            *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4) + 2) = seed >> 24;
            *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4) + 3) = 255;
        }

    printk("Rebooting in 2 sec");
    cpu_sleep(2000 * 1000);
    machine_reboot();

    panic("kmain_high end.");
}

void ap_kmain(struct cpu_core *core)
{
    // serial_putc(SERIAL_COM0, 'A');
    printk(KERN_INFO "booting CPU#%d", core->core_id);
    cpu_init_core(core->core_id);

    int seed = 0;
    while (1)
    {
        for (size_t x = (core->core_id * 50); x < (core->core_id * 50) + 50 && x < 800; x++)
            for (size_t y = (core->core_id * 50); y < (core->core_id * 50) + 50 && y < 600; y++)
            {
                seed = seed * 1664525 + 1013904223;
                *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4)) = seed >> 24;
                seed = seed * 1664525 + 1013904223;
                *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4) + 1) = seed >> 24;
                seed = seed * 1664525 + 1013904223;
                *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4) + 2) = seed >> 24;
                *((uint8_t *)0xfd000000 + y * (800 * 4) + (x * 4) + 3) = 255;
            }
    }
}

struct kernel_payload const *kernel_configuration()
{
    return &payload;
}
