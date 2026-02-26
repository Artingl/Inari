#ifdef CONFIG_DRV_RAMDISK_MULTIBOOT

#include <multiboot/multiboot.h>

#include <kernel/inari.h>
#include <kernel/errno.h>
#include <kernel/module.h>
#include <kernel/sys/block.h>
#include <kernel/sys/device.h>

#include <misc/string.h>

#define RAMDISK_BLOCK_SIZE 512

static int dev_off = 0;
static dev_t devices[16];

static int initrd_read(struct device *bdev, uint64_t lba, void *buf, size_t nblocks)
{
    if (!bdev || ! bdev->driver_data || !buf) return -EINVAL;
    multiboot_module_t *module = (multiboot_module_t*)bdev->driver_data;
    if ((lba * RAMDISK_BLOCK_SIZE + nblocks * RAMDISK_BLOCK_SIZE) >= bdev->size) return -EINVAL;
    // kprintf("%llu %llu", lba, nblocks);
    memcpy(buf, (void*)(module->mod_start + lba * RAMDISK_BLOCK_SIZE), nblocks * RAMDISK_BLOCK_SIZE);
    return 0;
}

static struct block_ops ops = {
    .read_blocks = &initrd_read
};

static int ramdisk_probe()
{
    bootinfo_t info = get_boot_info();
    multiboot_info_t *multiboot = (multiboot_info_t*)info.bootloader_info;
    multiboot_module_t *module;
    size_t i;

    if (info.bootloader_magic != MULTIBOOT_LKERNOADER_MAGIC)
        return -1;
    register_blkdev_group(RAMDISK_DRIVER, RAMDISK_BLOCK_SIZE, "ramdisk");

    if (multiboot->flags & MULTIBOOT_INFO_MODS)
    {
        for (i = 0; i < multiboot->mods_count && dev_off < 16; i++)
        {
            /* The module struct ad mods_addr isn't subject to change,  
               so we can pass it directly to our blkdev */
            module = &((multiboot_module_t *)multiboot->mods_addr)[i];
            if (!module->cmdline) continue;
            if (strcmp((char*)module->cmdline, "initrd") != 0)
                continue;
            register_blkdev(RAMDISK_DRIVER, &ops, module->mod_end - module->mod_start, module, &devices[dev_off++]);
        }
    }
    else kprintf("ramdisk: none found.");

    return 0;
}

static void ramdisk_cleanup()
{
    size_t i;
    for (i = 0; i < dev_off; i++)
        unregister_blkdev(devices[i]);
    unregister_blkdev_group(RAMDISK_DRIVER);
    dev_off = 0;
}

module_t ramdisk_module = {
    .probe = ramdisk_probe,
    .cleanup = ramdisk_cleanup
};

module_register(
    "ramdisk_multiboot",
    ramdisk_module
);

#endif