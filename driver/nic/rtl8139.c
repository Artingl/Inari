#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_SUBSYS_NET
#ifdef CONFIG_DRV_RTL8139

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/module.h>
#include <kernel/subsys/net.h>
#include <kernel/subsys/pci.h>

#include <misc/string.h>

#include <arch/sys.h>
#include <arch/x86/arch.h>

#define PCI_VENDOR 0x10EC
#define PCI_DEVICE 0x8139

#define REG_MAC0_5   0x00 // Size 6
#define REG_MAR0_7   0x08 // Size 8
#define REG_RBSTART  0x30 // Size 4
#define REG_CMD      0x37 // Size 1
#define REG_CAPR     0x38 // Size 2
#define REG_CBA      0x3A // Size 2
#define REG_ISR      0x3E // Size 2
#define REG_IMR      0x3C // Size 2
#define REG_TCR      0x40 // Size 4
#define REG_CONFIG_1 0x52 // Size 1
#define REG_RCR      0x44 // Size 4

#define REG_TX0     0x20 // Size 4
#define REG_TX1     0x24 // Size 4
#define REG_TX2     0x28 // Size 4
#define REG_TX3     0x2C // Size 4
#define REG_TX0_CMD 0x10 // Size 4
#define REG_TX1_CMD 0x14 // Size 4
#define REG_TX2_CMD 0x18 // Size 4
#define REG_TX3_CMD 0x1C // Size 4

#define RX_BUFFER_SIZE 8192

static dev_t net_dev = 0;
static struct pci_device *dev = NULL;
static uint16_t rx_offset;
static _lo_data uint8_t rx_buffer[RX_BUFFER_SIZE + 16 + 1500]; // 8KB + 16 bytes + 1500 bytes
static int tx_counter = 0;
static _lo_data uint8_t tx_buffer[4][1792];

struct rtl8139_packet_header {
    uint32_t rok : 1;   // Receive OK: When set, indicates that a good packet is received.
    uint32_t fae : 1;   // Frame Alignment Error: When set, indicates that a frame alignment error occurred on this
                        // received packet.
    uint32_t crc : 1;   // CRC Error: When set, indicates that a CRC error occurred on the received packet
    uint32_t longp : 1; // Long Packet: Set to 1 indicates that the size of the received packetexceeds 4k bytes.
    uint32_t runt : 1;  // Runt Packet Received: Set to 1 indicates that the received packet length is smaller than 64
                        // bytes ( i.e. media header + data + CRC < 64 bytes )
    uint32_t ise : 1; // Invalid Symbol Error: (100BASE-TX only) An invalid symbol was encountered during the reception
                      // of this packet if this bit set to 1.
    uint32_t reserved : 6;
    uint32_t bar : 1; // Broadcast Address Received: Set to 1 indicates that a broadcast packet is received. BAR, MAR
                      // bit will not be set simultaneously.
    uint32_t par : 1; // Physical Address Matched: Set to 1 indicates that the destination address of this packet
                      // matches the value written in ID registers.
    uint32_t mar : 1; // Multicast Address Received: Set to 1 indicates that a multicast packet is received.
} __attribute__((packed));

static int rtl8139_irq(uint32_t irq, void *driver_data) {
    if (!dev)
        return IRQ_HANDLED;

    struct rtl8139_packet_header *header;
    uint32_t status = x86_inw(dev->bar[0].base + REG_ISR);
    uint16_t length, cba_offset;
    /* Check if different device triggered the IRQ */
    if (status == 0x0)
        return IRQ_HANDLED;

    /* Clear TOK on successful send */
    if (status & (1 << 2)) {
        // status &= ~(1 << 2);
    }

    /* Check if have any packets (ROK) */
    if (status & (1 << 0)) {
        /* Loop while have any new packets */
        while (net_is_active(net_dev) && (cba_offset = x86_inw(dev->bar[0].base + REG_CBA)) != rx_offset) {
            rx_offset %= RX_BUFFER_SIZE;
            header = (struct rtl8139_packet_header *)&rx_buffer[rx_offset];
            length = *(uint16_t *)&rx_buffer[rx_offset + sizeof(struct rtl8139_packet_header)];

            if (header->rok) {
                net_rx_packet(net_dev, &rx_buffer[rx_offset + 4], length);
            }

            rx_offset = (rx_offset + length + 4 + 3) & ~3;
            x86_outw(dev->bar[0].base + REG_CAPR, rx_offset - 16);
        }
    }

    x86_outw(dev->bar[0].base + REG_ISR, status);
    return IRQ_HANDLED;
}

static int flush_tx_buffer(void *packet, size_t sz) {
    /* Try to find free buffer to use for transmission */
    int found_counter = -1, timeout = 10;

    do {
        uint32_t reg = x86_inl(dev->bar[0].base + REG_TX0_CMD + tx_counter * 4);
        /* Checking if OWN bit is set (idle) */
        if (reg & (1 << 13)) {
            found_counter = tx_counter;
            tx_counter = (tx_counter + 1) % 4;
            break;
        }

        tx_counter = (tx_counter + 1) % 4;
    } while (timeout-- > 0);

    if (found_counter == -1) {
        /* Didn't find any free buffer, drop */
        return -1;
    }

    /* Copy packet to tx buffer */
    memcpy(&tx_buffer[found_counter], packet, sz);

    /* The RTL8139 requires minimum 60 bytes of payload */
    if (sz < 60) {
        memset(tx_buffer[found_counter] + sz, 0, 60 - sz);
        sz = 60;
    }

    uint8_t tx = REG_TX0 + found_counter * 4;
    uint8_t tx_cmd = REG_TX0_CMD + found_counter * 4;

    /* Send physical address of the buffer to NIC */
    x86_outl(dev->bar[0].base + tx, (uint32_t)arch_virt_to_phys(arch_get_kernel_pagedir(), &tx_buffer[found_counter]));

    /* Specify the size of the data */
    x86_outl(dev->bar[0].base + tx_cmd, sz & 0x3fff);
    return 0;
}

static int rtl8139_tx(struct net_device *bdev, void *packet, uint32_t length) {
    if (!dev)
        return -ENODEV;
    int32_t signed_ln = (int32_t)length;

    do {
        if (flush_tx_buffer(packet + (length - signed_ln), MIN(signed_ln, 1792)) != 0)
            /* Dropping, return amount of unsent data */
            return MIN(signed_ln, 1792);
        signed_ln -= 1792;
    } while (signed_ln > 0);

    return 0;
}

static struct net_ops ops = {
    .tx = &rtl8139_tx,
};

static int rtl8139_probe() {
    int res = 0;
    if ((res = pci_find_next(&dev, PCI_DEVICE, PCI_VENDOR)) != 0)
        return res;
    memset(rx_buffer, 0, sizeof(rx_buffer));

    dev->command |= PCI_CMD_IO_SPACE;
    dev->command |= PCI_CMD_MEM_SPACE;
    dev->command |= PCI_CMD_BUS_MASTER;

    if ((res = pci_update_registers(dev)) != 0) {
        kprintf("rtl8139: setup failed.\n");
        return res;
    }

    if (dev->bar[0].type != PCI_TYPE_IO_SPACE) {
        kprintf("rtl8139: invalid IO space.\n");
        return res;
    }

    /* Power on and reset the NIC */
    x86_outb(dev->bar[0].base + REG_CONFIG_1, 0x00);
    x86_outb(dev->bar[0].base + REG_CMD, 0x10);
    while ((x86_inb(dev->bar[0].base + REG_CMD) & 0x10) != 0)
        ;

    /* Send the receive buffer physical memory location to the NIC */
    x86_outl(dev->bar[0].base + REG_RBSTART, (uint32_t)arch_virt_to_phys(arch_get_kernel_pagedir(), rx_buffer));
    rx_offset = 0;

    /* Setup IRQ (TOK and ROK bits high) */
    x86_outw(dev->bar[0].base + REG_IMR, 0x0005);

    /* Configuring receive buffer */
    /* AB - Accept Broadcast: Accept broadcast packets sent to mac ff:ff:ff:ff:ff:ff
     * AM - Accept Multicast: Accept multicast packets.
     * APM - Accept Physical Match: Accept packets send to NIC's MAC address.
     * AAP - Accept All Packets. Accept all packets (run in promiscuous mode). */
    x86_outl(dev->bar[0].base + REG_RCR, 0xf | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP

    /* Enable RX/TX (set the RE and TE bits high) */
    x86_outb(dev->bar[0].base + REG_CMD, 0x0C);

    /* * Set Transmit Configuration Register:
     * 0x03000000 = Standard Interframe Gap (IFG)
     * 0x00000700 = Maximum DMA burst size
     */
    x86_outl(dev->bar[0].base + REG_TCR, 0x03000700);

    uint8_t mac[6] = {
        x86_inb(dev->bar[0].base + REG_MAC0_5 + 0), x86_inb(dev->bar[0].base + REG_MAC0_5 + 1),
        x86_inb(dev->bar[0].base + REG_MAC0_5 + 2), x86_inb(dev->bar[0].base + REG_MAC0_5 + 3),
        x86_inb(dev->bar[0].base + REG_MAC0_5 + 4), x86_inb(dev->bar[0].base + REG_MAC0_5 + 5),
    };

    tx_counter = 0;
    irq_request(dev->irq, rtl8139_irq, NULL);
    return net_add_device(&net_dev, &ops, &mac[0], 1500, "rtl8139");
}

static void rtl8139_cleanup() {
    irq_free(dev->irq, rtl8139_irq);
    net_remove_device(net_dev);
    net_dev = 0;
    tx_counter = 0;
    dev = NULL;
}

module_t rtl8139_module = {
    .probe = rtl8139_probe,
    .cleanup = rtl8139_cleanup,
};

module_register("rtl8139", rtl8139_module);

#endif
#endif
#endif
