#include <kernel/kernel.h>

#include <drivers/ata/ata_pio.h>

#include <drivers/video/video.h>
#include <drivers/memory/memory.h>
#include <drivers/memory/vmm.h>
#include <drivers/cpu/cpu.h>
#include <drivers/cpu/amd/svm/svm.h>
#include <drivers/serial/serial.h>

#include <kernel/sys/console/console.h>
#include <kernel/sys/devfs/devfs.h>
#include <kernel/sys/disks/disks.h>
#include <kernel/sys/vfs/vfs.h>
#include <kernel/sys/sys.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

struct kernel_payload payload;

extern void *_kernel_start;
extern void *_kernel_end;

void __setup_virtualization();

void kmain(struct kernel_payload *__payload)
{
    const char *mount_point;
    struct devfs_node *block_device;
    struct disk *disk;

    memcpy(&payload, __payload, sizeof(struct kernel_payload));

    console_init();
    memory_init();
    memory_info();
    console_enable_heap();
    video_init();

    printk(KERN_INFO "Inari kernel (x86, %s)", payload.bootloader);
    printk(KERN_INFO "Kernel cmdline: %s", payload.cmdline);
    printk(KERN_INFO "Kernel virtual start: %p", &_kernel_start);
    printk(KERN_INFO "Kernel virtual end: %p", &_kernel_end);

    cpu_init();
    sys_init();

    // small memory check
    printk(KERN_INFO "=============== MEM TEST ===============");
    char *data = kmalloc(1024 * 1024 * 512);
    printk(KERN_INFO "kmalloc: %p (phys: %p)", data, vmm_get_phys(vmm_current_directory(), data));

    memory_info();
    kfree(data);
    memory_info();
    printk(KERN_INFO "===============   DONE   ===============");
    printk(KERN_INFO "Русский язык в консоли");

    kernel_assert(devfs_init() == DEVFS_SUCCESS);
    kernel_assert(vfs_init() == VFS_SUCCESS);

    // parse cmdline to initialize the kernel itself
    kernel_parse_cmdline();

    // mount the root
    mount_point = kernel_root_mount_point();
    if (mount_point == NULL)
        panic("kernel: unspecified mount point.");
    
    block_device = devfs_get_node(mount_point);
    if (block_device == NULL)
        panic("kernel: block device '%s' not found.", mount_point);    

    disk = alloc_disk(block_device);
    kernel_assert(vfs_mount_root(disk) == VFS_SUCCESS);

    // try to read from the disk device
    uint8_t buffer[SECTOR_SIZE];
    disk_read(disk, 0, 1, &buffer);

    printk(KERN_DEBUG "disk: 0x%x", buffer[0]);

    // __setup_virtualization();
    panic("kmain_high end.");
}

void __setup_virtualization()
{
    // initialize the SVM (if CPU is AMD)
    if (cpu_vendor_id() == CPU_AMD_VENDOR)
    {
        struct SVM *svm;
        int r;
        if ((r = cpu_svm_make(&svm)) == SVM_SUCCESS)
        {
            printk(KERN_NOTICE "CPU does support SVM.");
        }
        else
        {
            panic("Unable to init SVM: %d", svm);
        }
    }
    else
        panic("CPU is not an AMD");
}

struct kernel_payload const *kernel_configuration()
{
    return &payload;
}
