#include <kernel/kernel.h>

#include <drivers/ata/ata_pio.h>

#include <drivers/video/video.h>
#include <drivers/memory/memory.h>
#include <drivers/memory/vmm.h>
#include <drivers/cpu/cpu.h>
#include <drivers/cpu/amd/svm/svm.h>
#include <drivers/serial/serial.h>

#include <kernel/sys/console/console.h>
#include <kernel/sys/vfs/vfs.h>
#include <kernel/sys/sys.h>

#include <kernel/include/C/math.h>
#include <kernel/include/C/string.h>

struct kernel_payload payload;

extern void *_kernel_start;
extern void *_kernel_end;

void __setup_virtualization();
void kernel_loop();

void kmain(struct kernel_payload *__payload)
{
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
    printk(KERN_DEBUG "=============== MEM TEST ===============");
    char *data = kmalloc(1024 * 1024 * 512);
    printk(KERN_DEBUG "kmalloc: %p (phys: %p)", data, vmm_get_phys(vmm_current_directory(), data));

    memory_info();
    kfree(data);
    memory_info();
    printk(KERN_DEBUG "===============   DONE   ===============");

    printk("Русский язык в консоли");

    // initialize disk drivers
    ata_pio_init();


    vfs_init();

    // __setup_virtualization();

    kernel_loop();
    panic("kmain_high end.");
}

void kernel_loop()
{
    while (1)
    {
        // printk("SLEEP");
        // cpu_sleep(1000000);
    }
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