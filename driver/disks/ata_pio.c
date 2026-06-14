#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_ATA_PIO

#include <kernel/sys/console.h>
#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/module.h>
#include <kernel/timer.h>

#include <arch/sys.h>
#include <arch/x86/arch.h>
#include <misc/string.h>

#include <driver/disk/ata/ata.h>

static uint8_t ata_pio_ints_installed;

static int ata_pio_irq(uint32_t irq, void *driver_data) { return IRQ_HANDLED; }

static int ata_pio_read(struct ata_drive *drive, uint32_t lba, void *buf, size_t nsects) {
    uint8_t status;
    size_t i;

    uint16_t io_port = drive->controller ? ATA_SECONDARY_IO : ATA_PRIMARY_IO;
    uint8_t ata_drive = drive->drive ? ATA_SLAVE_DRIVE : ATA_MASTER_DRIVE;

    if (!buf || !drive || !drive->present)
        return -ENODEV;

    /* Select drive */
    x86_outb(io_port + ATA_DRIVE, (ata_drive + 0x40) | ((lba >> 24) & 0x0F));

    /* Send sector and count */
    x86_outb(io_port + ATA_SECTOR_CNT, (uint8_t)nsects);
    x86_outb(io_port + ATA_SECTOR_NUM, (uint8_t)lba);
    x86_outb(io_port + ATA_CYLINDER_LOW, (uint8_t)(lba >> 8));
    x86_outb(io_port + ATA_CYLINDER_HIGH, (uint8_t)(lba >> 16));

    /* "read sectors" command */
    x86_outb(io_port + ATA_COMMAND, 0x20);

    for (i = 0; i < nsects; i++) {
        /* Wait for the data by polling */
        do {
            status = x86_inb(io_port + ATA_STATUS);
            if (status & ATA_SR_ERR || status & ATA_SR_DF)
                break;
            timer_usleep(400); /* If scheduler is active, it will yield to avoid busylooping */
        } while (status & ATA_SR_BSY && (status & ATA_SR_DRQ) != ATA_SR_DRQ);

        /* Receive 256 16-bit values */
        x86_insw(io_port + ATA_DATA, &((uint16_t *)&((uint8_t *)buf)[0])[i * 256], 256);

        /* 400ns delay */
        timer_usleep(400); /* If scheduler is active, it will yield to avoid busylooping */
    }

    return 0;
}

static int ata_pio_write(struct ata_drive *drive, uint32_t lba, const void *buf, size_t nsects) { return -ENODEV; }

static struct ata_ops ata_pio_ops = {.read = &ata_pio_read, .write = &ata_pio_write};

void ata_pio_init(struct ata_drive *drive) {
    if (!ata_pio_ints_installed) {
        irq_request(14, &ata_pio_irq, NULL);
        // irq_request(15, &ata_pio_irq, NULL);
        ata_pio_ints_installed = 1;
    }

    drive->ops = &ata_pio_ops;
    return;
}

#endif
#endif
