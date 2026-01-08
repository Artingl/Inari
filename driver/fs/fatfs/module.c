#ifdef CONFIG_DRV_FATFS
#include <kernel/printk.h>
#include <kernel/fault/panic.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/module.h>
#include <kernel/sys/block.h>
#include <kernel/sys/vfs.h>
#include <kernel/errno.h>

#include <misc/string.h>

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

    return vfs_alloc_handle(mount, handle, fp);
}

static int fatfs_read(struct vfs_mount_point *mount, vfs_handle_t handle, void *buf, size_t len, size_t *rlen)
{
    uint32_t br;
    if (!rlen) rlen = &br;
    if (!mount) return -EINVAL;
    FIL *fp = vfs_handle_data(handle);
    return f_read(fp, mount->fs_data, buf, len, (UINT*)rlen) == FR_OK ? 0 : -ENOENT;
}

static int fatfs_size(struct vfs_mount_point *mount, vfs_handle_t handle, size_t *size)
{
    if (!mount) return -EINVAL;
    FIL *fp = vfs_handle_data(handle);
    *size = (size_t)fp->obj.objsize;
    return 0;
}

static int fatfs_readdir(struct vfs_mount_point *mount, const char *path, struct vfs_node *node, size_t offset)
{
    if (!mount || !path || !node) return -EINVAL;
    size_t file_id = 0, res = 0, ln;
    DIR dp;
    FILINFO fno;

    if (f_opendir(&dp, mount->fs_data, path) != FR_OK) return -ENOENT;
    for (;;)
    {
        f_readdir(&dp, mount->fs_data, &fno);
        if (fno.fname[0] == 0) break;
        if (file_id++ < node->off - offset) continue;

        ln = strlen(fno.fname);
        memcpy((void*)&node->name[0], (void*)fno.fname, ln > CONFIG_VFS_NAME_MAX ? CONFIG_VFS_NAME_MAX : ln);
        node->name[(ln > CONFIG_VFS_NAME_MAX ? (CONFIG_VFS_NAME_MAX - 1) : ln)] = '\0';

        if (fno.fattrib & AM_DIR)
        {
            node->st_mode = VFS_STAT_DIR;
            node->size = 0;
        }
        else {
            node->st_mode = VFS_STAT_FILE;
            node->size = fno.fsize;
        }

        res = 1;
        break;
    }

end:
    f_closedir(&dp, mount->fs_data);
    return res;
}

static struct vfs_layer_ops fatfs_ops = {
    .open = &fatfs_open,
    .close = &fatfs_close,
    .read = &fatfs_read,
    .size = &fatfs_size,
    .readdir = &fatfs_readdir
};

static struct vfs_layer fatfs_layer = {
    .name = "fatfs",
    .mount = &fatfs_mount_bdev,
    .unmount = &fatfs_unmount_bdev,
    .ops = &fatfs_ops
};

int fatfs_probe()
{
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