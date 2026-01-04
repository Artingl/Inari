#ifdef CONFIG_DRV_FATFS
#include <kernel/printk.h>
#include <kernel/fault/panic.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/module.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/errno.h>

/* FatFS implementation */
#include "ff.h"
#include "diskio.h"

DSTATUS ff_disk_status(dev_t pdrv)
{
	return RES_OK;
}

DSTATUS ff_disk_initialize(dev_t pdrv)
{
	return RES_OK;
}

DRESULT ff_disk_read(
	dev_t pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
    struct block_device *device = block_get(pdrv);
    if (!device) return RES_NOTRDY;
    device->ops->read_blocks(device, sector, (void*)buff, count);
	return RES_OK;
}

static int fatfs_unmount_bdev(struct vfs_mount_point *mount)
{
    if (!mount) return -EINVAL;
    kfree(mount->fs_data);
    f_unmount(mount->mount_point);
    return 0;
}

static int fatfs_mount_bdev(struct vfs_mount_point *mount)
{
    if (!mount) return -EINVAL;

    FATFS *fs = (FATFS*)kmalloc(sizeof(FATFS));
    FRESULT res;

    fs->pdrv = mount->bdev;
    res = f_mount(fs, mount->mount_point, 1);
    if (res == FR_OK)
    {
        switch (fs->fs_type)
        {
        case FS_FAT12:
            mount->fs_name = "FAT12"; break;
        case FS_FAT16:
            mount->fs_name = "FAT16"; break;
        case FS_FAT32:
            mount->fs_name = "FAT32"; break;
        case FS_EXFAT:
            mount->fs_name = "EXFAT"; break;
        default:
            mount->fs_name = "invalid";
            f_unmount(mount->mount_point);
            return -EINVFS;
        }

        mount->fs_data = fs;
        return 0;
    }

    return -EINVFS;
}

static int fatfs_close(struct vfs_mount_point *mount, vfs_handle_t handle)
{
    if (!mount) return -EINVAL;
    FIL *fp = vfs_handle_data(handle);
    int res = f_close(fp);
    if (fp) kfree(fp);
    return res == FR_OK ? 0 : -EINVAL;
}

static int fatfs_open(struct vfs_mount_point *mount, vfs_handle_t *handle, const char *path, int flags)
{
    int res;
    FIL *fp;
    if (!mount) return -EINVAL;

    fp = (FIL*)kmalloc(sizeof(FIL));
    /* NOTE: the flags provided by VFS are identical in meaning to those in fatfs */
    res = f_open(fp, (FATFS*)mount->fs_data, path, flags);
    if (res != FR_OK)
    {
        kfree(fp);
        return -ENOENT;
    }

    vfs_alloc_handle(mount, handle, fp);
    return 0;
}

static int fatfs_read(struct vfs_mount_point *mount, vfs_handle_t handle, void *buf, size_t len, size_t *rlen)
{
    uint32_t br;
    if (!rlen) rlen = &br;
    if (!mount) return -EINVAL;
    FIL *fp = vfs_handle_data(handle);
    return f_read(fp, mount->fs_data, buf, len, rlen) == FR_OK ? 0 : -EINVAL;
}

static struct vfs_layer_ops fatfs_ops = {
    .open = &fatfs_open,
    .close = &fatfs_close,
    .read = &fatfs_read,
};

static struct vfs_layer fatfs_layer = {
    .name = "fatfs",
    .mount = &fatfs_mount_bdev,
    .unmount = &fatfs_unmount_bdev,
    .ops = &fatfs_ops
};

int fatfs_probe()
{
    // FATFS fs;
    // FRESULT res;
    // DIR dp;
    // FIL fp;
    // FILINFO fno;

    // fs.pdrv = MKDEV(GPT_DRIVER, 1);
    // res = f_mount(&fs, "/", 1);
    // printk("fatfs: mount %d", res);

    // res = f_opendir(&dp, &fs, "/");
    // printk("fatfs: opendir %d", res);

    // for (;;) {
    //     res = f_readdir(&dp, &fs, &fno);            /* Read a directory item */
    //     if (fno.fname[0] == 0) break;          /* Error or end of dir */
    //     if (fno.fattrib & AM_DIR) {            /* It is a directory */
    //         printk("   <DIR>   %s", fno.fname);
    //     } else {                               /* It is a file */
    //         printk("   <FIL>   %10u %s", fno.fsize, fno.fname);
    //     }
    // }

    // char buff[128];
    // uint32_t b;

    // res = f_open(&fp, &fs, "/boot/grub/grub.cfg", FA_READ);
    // f_read(&fp, &fs, &buff[0], 128, &b);
    // printk("fatfs: open %d %s", res, buff);

    // struct block_device *device = block_get(MKDEV(GPT_DRIVER, 1));
    // if (!device)
    // {
    //     printk("fatfs: partition not found");
    //     return;
    // }

    // uint8_t buffer[512];
    // device->ops->read_blocks(device, 0, (void*)&buffer[0], 1);

    // printk("test: 0x%08x 0x%08x 0x%08x", buffer[0], buffer[1], buffer[2]);

    return vfs_add_layer(&fatfs_layer);
}

void fatfs_cleanup()
{
    vfs_remove_layer(&fatfs_layer);
}

module_t fatfs_module = {
    .probe = fatfs_probe,
    .cleanup = fatfs_cleanup
};

module_register(
    "fatfs",
    fatfs_module
);

#endif