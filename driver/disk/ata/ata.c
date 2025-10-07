#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_ATA

#include <kernel/inari.h>
#include <kernel/module.h>
#include <kernel/errno.h>
#include <kernel/block/block.h>

#include <driver/disk/ata/ata.h>

#include <misc/string.h>
#include <arch/x86/arch.h>
#include <arch/sys.h>

static struct ata_drive ata_drives[ATA_MAX_DRIVES];

static int ata_read_blocks(struct block_device *bdev, uint64_t lba, void *buf, size_t nblocks)
{
    return 0;
}

static int ata_write_blocks(struct block_device *bdev, uint64_t lba, const void *buf, size_t nblocks)
{
    return 0;
}

static struct block_ops ata_block_ops =
{
    .read_blocks = &ata_read_blocks,
    .write_blocks = &ata_write_blocks
};

// ‘int (*)(struct block_device *, uint64_t,  const void *, size_t)’ {aka ‘int (*)(struct block_device *, long long unsigned int,  const void *, unsigned int)’
// ‘int (*)(struct block_device *, uint64_t,  const void *, size_t)’ {aka ‘int (*)(struct block_device *, long long unsigned int,  const void *, unsigned int)’

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

    printk("ata: drive %s on %u[%u] %dKB; dma=%u, lba48=%u",
        drive->model, drive->drive, drive->controller, drive->size / 2,
        drive->supports_dma,
        drive->supports_lba48);
    
    /* We support only pio mode for now */
    ata_pio_init(drive);
    block_register_device(
        "hda", 0, 0, &ata_block_ops, (void*)drive
    );

    drive->present = 1;
    return 0;
}

int ata_probe()
{
    int ret;
    
    /* Identify all drives */
    ata_identify(0);
    ata_identify(1);
    ata_identify(2);
    ata_identify(3);

    // uint8_t buffer[512];
    // ata_drives[0].ops.read(&ata_drives[0], 0, (void*)&buffer, 1);
    // printk("ata: 0x%08x 0x%08x 0x%08x", (unsigned long)buffer[0], (unsigned long)buffer[1], (unsigned long)buffer[2]);

    return ret;
}

void ata_cleanup()
{
}

module_register(
    "ata",
    ata_probe,
    ata_cleanup
);

#endif
#endif