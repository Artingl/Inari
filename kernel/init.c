#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/timer.h>
#include <kernel/mm/pmm.h>
#include <kernel/mm/vmm.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/block.h>
#include <kernel/sys/char.h>
#include <kernel/sys/driver.h>
#include <kernel/proc/sched.h>
#include <kernel/proc/proc.h>
#include <kernel/module.h>
#include <kernel/sys/vfs.h>
#include <kernel/event.h>
#include <kernel/errno.h>

#include <arch/sys.h>
#include <arch/paging.h>
#include <misc/string.h>
#include <misc/format.h>

bootinfo_t bootinfo;
int init_task();
int mount_root();

#define assert(cond, fmt, ...) {if (!(cond)) {panic(fmt, ##__VA_ARGS__);}}

void kearly_init(bootinfo_t b)
{
    bootinfo = b;

    /* Initialize the early console for atleast some output */
    earlycon_init();
    printk("Inari kernel cmdline: %s", bootinfo.cmdline);

    assert(pmm_init() == 0, "pmm init failed.");
    assert(vmm_init() == 0, "vmm init failed.");
    assert(kmalloc_init() == 0, "kmalloc/kfree init failed.");

    printk("kearly_init: done");
}

void kmain(void)
{
    assert(event_bus_init() == 0, "event bus init failed.");
    assert(blkdev_init() == 0, "block device init failed.");
    assert(chardev_init() == 0, "char device init failed.");
    assert(sched_init() == 0, "scheduler init failed.");
    assert(vfs_init() == 0, "vfs init failed.");
    assert(proc_init() == 0, "proc init failed.");

    /* Initialize standard driver groups */
    register_chardev_group(TTY_DRIVER, "tty");
    register_chardev_group(SERIAL_DRIVER, "serial");
    register_chardev_group(KBD_DRIVER, "kbd");
    register_chardev_group(MOUSE_DRIVER, "mouse");

    assert(console_init() == 0, "console init failed.");

    enable_int();
    modules_init();

    assert(mount_root() == 0, "no root found.");
    assert(init_task() == 0, "unable to launch init.");
    sched_enter_core();
    panic("Reached end of kmain");
}

int mount_root()
{
    int res;
    vfs_handle_t file;
    
    char name_buff[ARG_MAX_LEN];
    char device[ARG_MAX_LEN];
    parse_cmdline_argument("root", &device[0]);

    /* Iterate through all block devices to find the required one */
    struct block_device *bdev;
    dev_t devs[128];
    int offset = 0, count = 0;
    while ((count = block_get_refs(&devs[0], offset, 128)) > 0)
    {
        for (; count > 0; count--)
        {
            bdev = block_get(devs[count - 1]);
            if (bdev)
            {
                sprintf(&name_buff[0], "%s%d", bdev->group->name, DEVID(bdev->dev));
                if (strcmp(name_buff, device) == 0)
                    goto found_dev;
            }
        }
        offset += 128;
    }

    return -ENODEV;
found_dev:
    printk("root: mounting %s as /", name_buff);
    return vfs_mount(bdev->dev, "/");
}

static void init_stub_thread()
{
    /* This thread is only created to delay a little the execution
     * of the init task, so critical kernel modules have time to initialize. */
    usleep(400000);

    vfs_mount(0, "/dev");

    char init_file[ARG_MAX_LEN];
    strcpy(&init_file[0], "/prog/init.exe");
    parse_cmdline_argument("init", &init_file[0]);
    
    pid_t pid;
    int res;
    if ((res = execp(&pid, init_file)) != 0)
        panic("init: Unable to launch init process: %s.", errstr[-res]);
}

int init_task()
{
    tid_t tid;
    return sched_create_thread(&tid, &init_stub_thread, NULL, NULL, NULL);
}

bootinfo_t get_boot_info()
{
    return bootinfo;
}
