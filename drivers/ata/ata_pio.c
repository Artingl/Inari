#include <kernel/kernel.h>

#include <drivers/cpu/cpu.h>

#include <drivers/ata/ata_pio.h>
#include <drivers/cpu/interrupts/interrupts.h>

interrupt_handler_t __ata_pio_primary_irq(struct regs32 *regs);
interrupt_handler_t __ata_pio_secondary_irq(struct regs32 *regs);

struct ata_pio_drive drives[4];

int ata_pio_init()
{
    cpu_interrupts_subscribe(&__ata_pio_primary_irq, ATA_PRIMARY_IRQ);
    cpu_interrupts_subscribe(&__ata_pio_secondary_irq, ATA_SECONDARY_IRQ);

    int master_res = ata_pio_identify(ATA_MASTER_DRIVE);
    int slave_res = ata_pio_identify(ATA_SLAVE_DRIVE);

    if (master_res != ATA_SUCCESS && slave_res != ATA_SUCCESS)
    {
        printk(KERN_WARNING "ATA PIO: Unable to find master (status = %d) and slave drives (status = %d).",
               master_res, slave_res);
        return 1;
    }
    else if (master_res != ATA_SUCCESS)
    {
        printk(KERN_NOTICE "ATA PIO: Master drive not found.");
    }
    else if (slave_res != ATA_SUCCESS)
    {
        printk(KERN_NOTICE "ATA PIO: Slave drive not found.");
    }
    else
    {
        printk(KERN_INFO "ATA PIO: Master and slave drivers were found.");
    }

    uint8_t buffer[512];
    memset(&buffer[0], 0, 512);
    ata_pio_read28(&drives[ATA_DRIVE_ID(ATA_MASTER_DRIVE, 0x00)], 0, 1, (uint16_t *)&buffer[0]);
    printk("%p %p %p", (unsigned long)buffer[0], (unsigned long)buffer[1], (unsigned long)buffer[2]);

    return 0;
}

int ata_pio_identify(uint8_t drive_id)
{
    size_t i;
    struct ata_pio_drive *drive = &drives[ATA_DRIVE_ID(drive_id, 0x00)];
    drive->drive_id = drive_id;
    drive->controller = 0;

    // select master drive
    __outb(ATA_PRIMARY_IO(ATA_DRIVE), drive->drive_id);

    // set sectorcount, lbalo, lbamid, and lbahi
    __outb(ATA_PRIMARY_IO(ATA_SECTOR_CNT), 0);
    __outb(ATA_PRIMARY_IO(ATA_SECTOR_NUM), 0);
    __outb(ATA_PRIMARY_IO(ATA_CYLINDER_LOW), 0);
    __outb(ATA_PRIMARY_IO(ATA_CYLINDER_HIGH), 0);

    // send identify
    // TODO: if we use PIC, after making this outb we'll get the INVALID_OPCODE exception for some reason...
    __outb(ATA_PRIMARY_IO(ATA_COMMAND), 0xEC);
    // uint16_t port = 0x1f7; uint8_t val = 0xec;
    // __asm__ ( "outb %0, %1" : : "a"(val), "Nd"(port) :"memory");
    // panic("FUCK");

    // check if drive exists
    uint8_t status = __inb(ATA_PRIMARY_IO(ATA_STATUS));
    if (status == 0)
    {
        return ATA_DRIVE_NOT_FOUND;
    }

    // poll status port for BSY to clear
    while (status & BSY)
    {
        status = __inb(ATA_PRIMARY_IO(ATA_STATUS));

        // if any of these ports are non-zero, the drive is not ATA
        if (__inb(ATA_PRIMARY_IO(ATA_CYLINDER_LOW)) != 0 || __inb(ATA_PRIMARY_IO(ATA_CYLINDER_HIGH)) != 0)
        {
            return ATA_NOT_ATA_DRIVE;
        }
    }

    // poll status port for DRQ or for ERR to set
    while (!(status & DRQ) && !(status & ERR))
    {
        status = __inb(ATA_PRIMARY_IO(ATA_STATUS));

        // if any of these ports are non-zero, the drive is not ATA
        if (__inb(ATA_PRIMARY_IO(ATA_CYLINDER_LOW)) != 0 || __inb(ATA_PRIMARY_IO(ATA_CYLINDER_HIGH)) != 0)
        {
            return ATA_NOT_ATA_DRIVE;
        }
    }

    if (status & ERR)
    {
        return ATA_ERROR;
    }

    __insw(ATA_PRIMARY_IO(ATA_DATA), (uint16_t*)&drive->payload[0], 256);

    return ATA_SUCCESS;
}

int ata_pio_read28(struct ata_pio_drive *drive, uint32_t sector, uint8_t count, uint16_t *buffer)
{
    uint8_t status;
    size_t i;

    // select drive
    __outb(ATA_PRIMARY_IO(ATA_DRIVE), (drive->drive_id + 0x40) | ((sector >> 24) & 0x0F));

    // send sector and count
    __outb(ATA_PRIMARY_IO(ATA_SECTOR_CNT), (uint8_t)count);
    __outb(ATA_PRIMARY_IO(ATA_SECTOR_NUM), (uint8_t)sector);
    __outb(ATA_PRIMARY_IO(ATA_CYLINDER_LOW), (uint8_t)(sector >> 8));
    __outb(ATA_PRIMARY_IO(ATA_CYLINDER_HIGH), (uint8_t)(sector >> 16));

    // "read sectors" command
    __outb(ATA_PRIMARY_IO(ATA_COMMAND), 0x20);

    for (i = 0; i < count; i++)
    {
        // Wait for the data by polling
        // TODO: Polling is a bad idea... Use IRQ please.
        do
        {
            status = __inb(ATA_PRIMARY_IO(ATA_STATUS));
            if (status & ERR || status & DF)
                break;
        } while (status & BSY && (status & DRQ) != DRQ);

        // Receive 256 16-bit values
        __insw(ATA_PRIMARY_IO(ATA_DATA), &buffer[i * 256], 256);

        // 400ns delay
        // TODO: We need to implement proper IRQ as said above
        cpu_sleep(400);
    }

    return ATA_SUCCESS;
}

interrupt_handler_t __ata_pio_primary_irq(struct regs32 *regs)
{
}

interrupt_handler_t __ata_pio_secondary_irq(struct regs32 *regs)
{
}