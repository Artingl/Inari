#include <kernel/kernel.h>

#include <drivers/ata/ata_pio.h>
#include <drivers/cpu/interrupts/interrupts.h>

interrupt_handler_t __ata_pio_primary_irq(struct regs32 *regs);
interrupt_handler_t __ata_pio_secondary_irq(struct regs32 *regs);

struct ata_pio_drive drives[4];

int ata_pio_init()
{
    cpu_interrupts_subscribe(&__ata_pio_primary_irq, ATA_PIO_PRIMARY_IRQ);
    cpu_interrupts_subscribe(&__ata_pio_secondary_irq, ATA_PIO_SECONDARY_IRQ);

    int master_res = ata_pio_identify(ATA_MASTER_DRIVE);
    int slave_res = ata_pio_identify(ATA_SLAVE_DRIVE);

    if (master_res != 0 && slave_res != 0)
    {
        printk(KERN_WARNING "ATA PIO: Unable to find master (status = %d) and slave drives (status = %d).",
               master_res, slave_res);
        return 1;
    }
    else if (master_res != 0)
    {
        printk(KERN_NOTICE "ATA PIO: Master drive not found.");
    }
    else if (slave_res != 0)
    {
        printk(KERN_NOTICE "ATA PIO: Slave drive not found.");
    }
    else
    {
        printk(KERN_INFO "ATA PIO: Master and slave drivers were found.");
    }

    return 0;
}

int ata_pio_identify(uint8_t drive_id)
{
    size_t i;
    struct ata_pio_drive *drive = &drives[ATA_PIO_DRIVE_ID(drive_id, 0x00)];
    drive->drive_id = drive_id;
    drive->controller = 0;

    // select master drive
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_DRIVE), drive->drive_id);

    // set sectorcount, lbalo, lbamid, and lbahi
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_SECTOR_CNT), 0);
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_SECTOR_NUM), 0);
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_CYLINDER_LOW), 0);
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_CYLINDER_HIGH), 0);

    // send identify
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_COMMAND), 0xec);

    // check if drive exists
    uint8_t status = __inb(ATA_PIO_PRIMARY_IO(ATA_PIO_STATUS));
    if (status == 0)
    {
        return ATA_PIO_DRIVE_NOT_FOUND;
    }

    // poll status port for BSY to clear
    while (status & BSY)
    {
        status = __inb(ATA_PIO_PRIMARY_IO(ATA_PIO_STATUS));

        // if any of these ports are non-zero, the drive is not ATA
        if (__inb(ATA_PIO_PRIMARY_IO(ATA_PIO_CYLINDER_LOW)) != 0 || __inb(ATA_PIO_PRIMARY_IO(ATA_PIO_CYLINDER_HIGH)) != 0)
        {
            return ATA_PIO_NOT_ATA_DRIVE;
        }
    }

    // poll status port for DRQ or for ERR to set
    while (!(status & DRQ) && !(status & ERR))
    {
        status = __inb(ATA_PIO_PRIMARY_IO(ATA_PIO_STATUS));

        // if any of these ports are non-zero, the drive is not ATA
        if (__inb(ATA_PIO_PRIMARY_IO(ATA_PIO_CYLINDER_LOW)) != 0 || __inb(ATA_PIO_PRIMARY_IO(ATA_PIO_CYLINDER_HIGH)) != 0)
        {
            return ATA_PIO_NOT_ATA_DRIVE;
        }
    }

    if (status & ERR)
    {
        return ATA_PIO_ERROR;
    }

    drive->present = true;

    for (i = 0; i < 256; i++)
    {
        drive->payload[i] = __inw(ATA_PIO_PRIMARY_IO(ATA_PIO_DATA));
    }

    printk("%x", ((uint16_t*)drive->payload)[83]);

    return 0;
}

void ata_pio_read28(struct ata_pio_drive *drive, size_t sector, uint16_t *buffer)
{
    // select drive
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_DRIVE), (drive->drive_id + 0x40) | ((LBA >> 24) & 0x0F));

    // send sector
    __outb(ATA_PIO_PRIMARY_IO(ATA_PIO_SECTOR_CNT), sector);


}

interrupt_handler_t __ata_pio_primary_irq(struct regs32 *regs)
{
}

interrupt_handler_t __ata_pio_secondary_irq(struct regs32 *regs)
{
}
