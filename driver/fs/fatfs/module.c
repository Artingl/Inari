#ifdef CONFIG_DRV_FATFS
#include "kernel/printk.h"
#include "kernel/module.h"
#include "kernel/sys/block.h"

/* FatFS implementation */
#include "ff.h"			/* Basic definitions of FatFs */
#include "diskio.h"		/* Declarations FatFs MAI */

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	// switch (pdrv) {
	// case DEV_RAM :
	// 	result = RAM_disk_status();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_MMC :
	// 	result = MMC_disk_status();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_USB :
	// 	result = USB_disk_status();

	// 	// translate the reslut code here

	// 	return stat;
	// }
	return 0;
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	// switch (pdrv) {
	// case DEV_RAM :
	// 	result = RAM_disk_initialize();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_MMC :
	// 	result = MMC_disk_initialize();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_USB :
	// 	result = USB_disk_initialize();

	// 	// translate the reslut code here

	// 	return stat;
	// }
	return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;

    struct block_device *device = block_get(MKDEV(GPT_DRIVER, 1));
    if (!device)
    {
        printk("fatfs: partition not found");
        return RES_NOTRDY;
    }

    device->ops->read_blocks(device, sector, (void*)buff, count);

	return RES_OK;
}

int fatfs_probe()
{
    FATFS fs;
    FRESULT res;
    DIR dp;
    FIL fp;
    FILINFO fno;

    res = f_mount(&fs, "/", 1);
    printk("fatfs: mount %d", res);

    res = f_opendir(&dp, "/boot");
    printk("fatfs: opendir %d", res);

    for (;;) {
        res = f_readdir(&dp, &fno);            /* Read a directory item */
        if (fno.fname[0] == 0) break;          /* Error or end of dir */
        if (fno.fattrib & AM_DIR) {            /* It is a directory */
            printk("   <DIR>   %s", fno.fname);
        } else {                               /* It is a file */
            printk("   <FIL>   %10u %s", fno.fsize, fno.fname);
        }
    }


    // res = f_open(&fp, "/boot/grub/grub.cfg", FA_READ);
    // printk("fatfs: open %d", res);

    // struct block_device *device = block_get(MKDEV(GPT_DRIVER, 1));
    // if (!device)
    // {
    //     printk("fatfs: partition not found");
    //     return;
    // }

    // uint8_t buffer[512];
    // device->ops->read_blocks(device, 0, (void*)&buffer[0], 1);

    // printk("test: 0x%08x 0x%08x 0x%08x", buffer[0], buffer[1], buffer[2]);

    return 0;
}

void fatfs_cleanup()
{
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