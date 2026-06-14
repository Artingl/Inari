#include <kernel/sys/console.h>
#include <kernel/errno.h>
#include <kernel/event.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/module.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/sched.h>
#include <kernel/sys/block.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/vfs.h>
#include <kernel/timer.h>

#include <arch/paging.h>
#include <arch/sys.h>
#include <misc/format.h>
#include <misc/string.h>

#ifdef CONFIG_SUBSYS_PCI
#include <kernel/subsys/pci.h>
#endif

#ifdef CONFIG_SUBSYS_NET
#include <kernel/subsys/net.h>
#endif

#ifdef CONFIG_SUBSYS_HID
#include <kernel/subsys/hid.h>
#endif

#ifdef CONFIG_SUBSYS_VIDEO
#include <kernel/subsys/video.h>
#endif

bootinfo_t bootinfo;
int init_task();
int mount_root();

#define assert(cond, fmt, ...)                                                                                         \
    {                                                                                                                  \
        if (!(cond)) {                                                                                                 \
            panic(fmt, ##__VA_ARGS__);                                                                                 \
        }                                                                                                              \
    }

void kearly_init() {
    /* Initialize the early console for atleast some output */
    kprintf("Inari kernel cmdline: %s", bootinfo.cmdline);

    assert(pmm_init() == 0, "pmm init failed.");
    assert(vmm_init() == 0, "vmm init failed.");
    assert(kmalloc_init() == 0, "kmalloc/kfree init failed.");

    kprintf("kearly_init: done");
}

void kmain(void) {
    assert(event_bus_init() == 0, "event bus init failed.");
    assert(blkdev_init() == 0, "block device init failed.");
    assert(chardev_init() == 0, "char device init failed.");
    assert(sched_init() == 0, "scheduler init failed.");
    assert(vfs_init() == 0, "vfs init failed.");
    assert(proc_init() == 0, "proc init failed.");

    /* Initialize standard driver groups */
    register_chardev_group(TTY_DRIVER, "tty");
    register_chardev_group(SERIAL_DRIVER, "serial");

#ifdef CONFIG_SUBSYS_PCI
    assert(pci_init() == 0, "pci init failed.");
#endif

#ifdef CONFIG_SUBSYS_NET
    assert(net_init() == 0, "net init failed.");
#endif

#ifdef CONFIG_SUBSYS_HID
    assert(hid_init() == 0, "hid subsys init failed.");
#endif

#ifdef CONFIG_SUBSYS_VIDEO
    assert(video_init() == 0, "video subsys init failed.");
#endif

    assert(console_init() == 0, "console init failed.");
    assert(init_task() == 0, "unable to launch init.");

    sched_enter_core();
    panic("Reached end of kmain");
}

int mount_root() {
    char name_buff[ARG_MAX_LEN];
    char device[ARG_MAX_LEN];
    int found_root = parse_cmdline_argument("root", &device[0]) == 0;

    /* Iterate through all block devices to find the required one */
    struct device *bdev;
    dev_t devs[128];
    int offset = 0, count = 0;
    while ((count = block_get_refs(&devs[0], offset, 128)) > 0) {
        for (; count > 0; count--) {
            bdev = block_get(devs[count - 1]);
            if (bdev) {
                sprintf(&name_buff[0], "%s%d", bdev->group->name, DEVID(bdev->dev));

                /* If specified root blkdev, use it */
                if (found_root && strcmp(name_buff, device) == 0) {
                    kprintf("init: mounting %s as /", name_buff);
                    return vfs_mount(bdev->dev, "/");
                }
                /* Otherwise use any firstly found and successfully mounted */
                else if (!found_root && vfs_mount(bdev->dev, "/") == 0) {
                    kprintf("init: mounted on %s as /", name_buff);
                    return 0;
                }
            }
        }
        offset += 128;
    }

    return -ENODEV;
}

static void init_stub_thread() {
    /* Initialize the rest of the kernel and mount root */
    modules_init();
    assert(mount_root() == 0, "no root found.");
    assert(vfs_mount(0, "/devices") == 0, "devfs mount failed.");

    char init_file[ARG_MAX_LEN];
    strcpy(&init_file[0], "/programs/init.exe");
    parse_cmdline_argument("init", &init_file[0]);

    pid_t pid;
    int res;
    kprintf("init: running init at %s", init_file);
    disable_int();
    if ((res = execp(&pid, init_file)) != 0)
        panic("init: unable to launch init process: %s.", errstr[-res]);
    enable_int();
}

int init_task() {
    tid_t tid;
    return sched_create_thread("init_stub", &tid, &init_stub_thread, NULL, NULL, NULL);
}

bootinfo_t get_boot_info() { return bootinfo; }

void kpower_off(uint8_t do_reboot) {
    /* TODO: graceful shutdown */
#ifdef CONFIG_SUBSYS_VIDEO
    video_disable();
#endif

    console_switch_early();
    kprintf("kernel: shutting down (do_reboot = %d)", do_reboot);
    sched_stop();
    modules_cleanup();

    if (do_reboot)
        reboot();
    else
        poweroff();
}
