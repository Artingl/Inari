#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_ATA

#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/errno.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/device.h>
#include <kernel/sys/block.h>

#include <driver/disk/ata/ata.h>

#include <misc/string.h>
#include <arch/x86/arch.h>
#include <arch/sys.h>

static struct ata_drive ata_drives[ATA_MAX_DRIVES];

static int ata_read_blocks(struct device *bdev, uint64_t lba, void *buf, size_t nblocks)
{
    struct ata_drive *drive = bdev->driver_data;
    if (!drive) return -ENODEV;
    return drive->ops->read(drive, lba, buf, nblocks);
}

static int ata_write_blocks(struct device *bdev, uint64_t lba, const void *buf, size_t nblocks)
{
    struct ata_drive *drive = bdev->driver_data;
    if (!drive) return -ENODEV;
    return drive->ops->write(drive, lba, buf, nblocks);
}

static struct block_ops ata_block_ops = {
    .read_blocks = &ata_read_blocks,
    .write_blocks = &ata_write_blocks
};

static int ata_identify(uint8_t drive_id)
{
    size_t i;
    uint8_t payload[512];
    struct ata_drive *drive = &ata_drives[drive_id];
    drive->present = 0;
    drive->controller = drive_id < 2 ? 0 : 1;
    drive->drive = drive_id % 2;

    uint16_t io_port = drive->controller ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint16_t ctrl_port = drive->drive ? ATA_SECONDARY_CTRL : ATA_PRIMARY_CTRL;
    uint8_t ata_drive = drive->drive ? ATA_SLAVE_DRIVE : ATA_MASTER_DRIVE;

    /* Select drive */
    x86_outb(io_port + ATA_DRIVE, ata_drive);

    for (i = 0; i < 4; i++)
        x86_inb(ctrl_port);

    /* Check if drive exists */
    uint8_t status = x86_inb(io_port + ATA_STATUS);
    if (status == 0)
        return -ENODEV;

    /* Set sectorcount, lbalo, lbamid, and lbahi */
    x86_outb(io_port + ATA_SECTOR_CNT, 0);
    x86_outb(io_port + ATA_SECTOR_NUM, 0);
    x86_outb(io_port + ATA_CYLINDER_LOW, 0);
    x86_outb(io_port + ATA_CYLINDER_HIGH, 0);

    for (i = 0; i < 4; i++)
        x86_inb(ctrl_port);

    /* Send identify */
    x86_outb(io_port + ATA_COMMAND, 0xEC);

    /* Poll status port for BSY to clear */
    for (size_t i = 0; i < 1000000 && (status & ATA_SR_BSY); i++)
        status = x86_inb(io_port + ATA_STATUS);
    if (status & ATA_SR_BSY) return -ETIMEDOUT;

    /* Poll status port for DRQ or for ERR to set */
    do
    {
        status = x86_inb(io_port + ATA_STATUS);

        /* If any of these ports are non-zero, the drive is not ATA */
        if (x86_inb(io_port + ATA_CYLINDER_LOW) != 0 || x86_inb(io_port + ATA_CYLINDER_HIGH) != 0)
            return -ENODEV;
    }
    while (!(status & ATA_SR_DRQ) && !(status & ATA_SR_ERR));

    if (status & ATA_SR_ERR)
        return -EIO;
    
    x86_insw(io_port + ATA_DATA, (uint16_t*)&payload[0], 256);

    drive->supports_dma   = (uint8_t)(*((uint16_t*)(&payload[ATA_IDENT_CAPABILITIES])) & (1 << 8));
    drive->supports_lba48 = (uint8_t)(*((uint32_t*)(&payload[ATA_IDENT_COMMANDSETS])) & (1 << 26));

    /* Retrieve drive size */
    if (drive->supports_lba48)
        drive->size = *((uint32_t*)(&payload[ATA_IDENT_MAX_LBA_EXT]));
    else
        drive->size = *((uint32_t*)(&payload[ATA_IDENT_MAX_LBA]));

    /* Retrieve the model name */
    int8_t s_start = -1;
    for(i = 0; i < 40; i += 2)
    {
        drive->model[i] = payload[54 + i + 1];
        drive->model[i + 1] = payload[54 + i];
        if (drive->model[i + 1] == ' ' && s_start == -1)
            s_start = i;
        else if (drive->model[i + 1] != ' ')
            s_start = -1;
    }
    if (s_start == -1) s_start = 39;
    drive->model[s_start+1] = '\0';
    drive->present = 1;

    kprintf("ata: drive %s on %u[%u] %dKB; dma=%u, lba48=%u",
        drive->model, drive->drive, drive->controller, drive->size / 2,
        drive->supports_dma,
        drive->supports_lba48);
    
    /* We support only pio mode for now */
    ata_pio_init(drive);

    register_blkdev(
        ATA_DRIVER,
        &ata_block_ops,
        drive->size * 512,
        (void*)drive, NULL);
    return 0;
}

static int ata_probe()
{
    int ret = 0;

    register_blkdev_group(ATA_DRIVER, ATA_BLOCK_SIZE, "hd");
    
    /* Identify all drives */
    ata_identify(0);
    ata_identify(1);
    ata_identify(2);
    ata_identify(3);
    
    return ret;
}

static void ata_cleanup()
{
    unregister_blkdev_group(ATA_DRIVER);
}

module_t ata_module = {
    .probe = ata_probe,
    .cleanup = ata_cleanup
};

module_register(
    "ata",
    ata_module
);

#endif
#endif