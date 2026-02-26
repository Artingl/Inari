#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_PS2

#include <kernel/inari.h>
#include <kernel/timer.h>
#include <kernel/console/console.h>
#include <kernel/subsys/kbd.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/errno.h>
#include <kernel/module.h>
#include <kernel/interrupts/irq.h>

#include <misc/string.h>
#include <arch/sys.h>
#include <arch/x86/arch.h>

#define PS2_KBD_IRQ   0x1
#define PS2_MOUSE_IRQ 0x12

static int ps2_is_dual = 0;

static int ps2_devices_off = 0;
static struct {
    uint8_t port;   // 0 - disabled; 1 - first; 2 - second
    uint8_t type;   // 1 - keyboard; 2 - mouse
    dev_t dev;
} ps2_devices[4] = {0};

uint16_t scancode_set1_map[128] = {
    [0x00] = KEY_NONE,
    [0x01] = KEY_ESC,
    [0x02] = '1',
    [0x03] = '2',
    [0x04] = '3',
    [0x05] = '4',
    [0x06] = '5',
    [0x07] = '6',
    [0x08] = '7',
    [0x09] = '8',
    [0x0A] = '9',
    [0x0B] = '0',
    [0x0C] = '-',
    [0x0D] = '=',
    [0x0E] = KEY_BACKSPACE,
    [0x0F] = KEY_TAB,
    [0x10] = 'q',
    [0x11] = 'w',
    [0x12] = 'e',
    [0x13] = 'r',
    [0x14] = 't',
    [0x15] = 'y',
    [0x16] = 'u',
    [0x17] = 'i',
    [0x18] = 'o',
    [0x19] = 'p',
    [0x1A] = '[',
    [0x1B] = ']',
    [0x1C] = KEY_ENTER,
    [0x1D] = KEY_LCTRL,
    [0x1E] = 'a',
    [0x1F] = 's',
    [0x20] = 'd',
    [0x21] = 'f',
    [0x22] = 'g',
    [0x23] = 'h',
    [0x24] = 'j',
    [0x25] = 'k',
    [0x26] = 'l',
    [0x27] = ';',
    [0x28] = '\'',
    [0x29] = '`',
    [0x2A] = KEY_LSHIFT,
    [0x2B] = '\\',
    [0x2C] = 'z',
    [0x2D] = 'x',
    [0x2E] = 'c',
    [0x2F] = 'v',
    [0x30] = 'b',
    [0x31] = 'n',
    [0x32] = 'm',
    [0x33] = ',',
    [0x34] = '.',
    [0x35] = '/',
    [0x36] = KEY_RSHIFT,
    [0x37] = KEY_KP_STAR,
    [0x38] = KEY_LALT,
    [0x39] = ' ',
    [0x3A] = KEY_CAPSLOCK,
    [0x3B] = KEY_F1,
    [0x3C] = KEY_F2,
    [0x3D] = KEY_F3,
    [0x3E] = KEY_F4,
    [0x3F] = KEY_F5,
    [0x40] = KEY_F6,
    [0x41] = KEY_F7,
    [0x42] = KEY_F8,
    [0x43] = KEY_F9,
    [0x44] = KEY_F10,
    [0x45] = KEY_NUMLOCK,
    [0x46] = KEY_SCROLLLOCK,
    [0x47] = KEY_KP_HOME,
    [0x48] = KEY_KP_UP,
    [0x49] = KEY_KP_PGUP,
    [0x4A] = KEY_KP_MINUS,
    [0x4B] = KEY_KP_LEFT,
    [0x4C] = '5',
    [0x4D] = KEY_KP_RIGHT,
    [0x4E] = KEY_KP_PLUS,
    [0x4F] = KEY_KP_END,
    [0x50] = KEY_KP_DOWN,
    [0x51] = KEY_KP_PGDN,
    [0x52] = KEY_KP_INS,
    [0x53] = KEY_KP_DEL,
    [0x54] = KEY_NONE,
    [0x55] = KEY_NONE,
    [0x56] = KEY_NONE,
    [0x57] = KEY_F11,
    [0x58] = KEY_F12,
};

static struct kbd_event last_kbd_event = {0};

static int ps2_kbd_irq(uint32_t irq, void *dev_id)
{
    uint8_t in = x86_inb(0x60);
    int released = in & 0x80;
    uint8_t code = in & 0x7F;
    uint16_t key = scancode_set1_map[code];
    last_kbd_event.event_id++;
    last_kbd_event.released = released;
    last_kbd_event.code = code;
    last_kbd_event.key = key;
    // x86_outb(0x64, 0);
    return IRQ_HANDLED;
}

static int ps2_mouse_irq(uint32_t irq, void *dev_id)
{
    uint8_t in = x86_inb(0x60);
    // printk("0x%x", in);
    return IRQ_HANDLED;
}

static int kbd_read(struct device *chardev, uint8_t *buf, size_t sz)
{
    if (sz > sizeof(struct kbd_event))
        sz = sizeof(struct kbd_event);

    memcpy((void*)buf, (void*)&last_kbd_event, sz);
    return sz;
}

static struct char_ops ps2_kbd_ops = {
    .read = &kbd_read,
};

static int ps2_kbd_init(uint8_t port)
{
    int res;
    uint8_t in, b0, b1;

    /* Check that we have PS/2 keyboard */
    if (port == 2) x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xEE);
    while ((in = x86_inb(0x60)) == 0xFE)
        usleep(0x1000);
    if (in != 0xEE) return -ENODEV;

    /* Identify keyboard */
    if (port == 2) x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xF5);
    while ((in = x86_inb(0x60)) == 0xFE) {
        usleep(0x1000);
        x86_outb(0x60, 0xF5);
    }
    if (port == 2) x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xF2);
    while ((in = x86_inb(0x60)) == 0xFE) {
        usleep(0x1000);
    if (port == 2) x86_outb(0x64, 0xD4);
        x86_outb(0x60, 0xF2);
    }
    b0 = x86_inb(0x60);
    usleep(0x1000);
    b1 = x86_inb(0x60);
    usleep(0x1000);
    if (port == 2) x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xF4);
    while ((in = x86_inb(0x60)) == 0xFE) {
        usleep(0x1000);
        if (port == 2) x86_outb(0x64, 0xD4);
        x86_outb(0x60, 0xF4);
    }

    if ((res = register_chardev(KBD_DRIVER, &ps2_kbd_ops, NULL, &last_kbd_event.dev)) != 0)
        return res;

    printk("ps2: initalized keyboard on port %d", port);
    last_kbd_event.event_id = 0;
    ps2_devices[ps2_devices_off].port = port;
    ps2_devices[ps2_devices_off].type = 1;
    ps2_devices[ps2_devices_off++].dev = last_kbd_event.dev;

    return 0;
}

static int ps2_mouse_init(uint8_t port)
{
    uint8_t in;

    /* Check that we have PS/2 keyboard */
    if (port == 2) x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xFF);
    while ((in = x86_inb(0x60)) == 0xFE)
        usleep(0x1000);
    if (in != 0xFA) return -ENODEV;

    ps2_devices[ps2_devices_off].port = port;
    ps2_devices[ps2_devices_off++].type = 2;
    // ps2_devices[ps2_devices_off].dev = ;

    printk("ps2: initalized mouse on port %d", port);
    return 0;
}

static int ps2_init()
{
    uint8_t in;
    int timeout;
    ps2_is_dual = 0;

    /* Disable devices */
    x86_outb(0x64, 0xAD);
    x86_outb(0x64, 0xA7);
    
    /* Flush the output buffer */
    x86_inb(0x60);

    /* Set the controller configuration byte */  
    x86_outb(0x64, 0x20);
    while (!(x86_inb(0x64) & (1 << 0)))
        usleep(0x1000);
    in = x86_inb(0x60);
    x86_outb(0x64, 0x60);
    /* Disabling IRQs for port 1; Enabling clock signal */
    x86_outb(0x60, in & ~((1 << 0) | (1 << 4)));

    /* Perform Controller Self Test */
    x86_outb(0x64, 0xAA);
    while (!(x86_inb(0x64) & (1 << 0)))
        usleep(0x1000);
    in = x86_inb(0x60);
    if (in != 0x55)
    {
        printk("ps2: self-test failed.");
        return -ENODEV;
    }
    
    /* Determine if there are 2 channels */
    x86_outb(0x64, 0xA8);
        x86_outb(0x64, 0x20);
    while (!(x86_inb(0x64) & (1 << 0)))
        usleep(0x1000);
    in = x86_inb(0x60);
    if (ps2_is_dual = !(in & (1 << 5)))
    {
        /* Second port available. Disable again */
        x86_outb(0x64, 0xA7);

        x86_outb(0x64, 0x20);
        while (!(x86_inb(0x64) & (1 << 0)))
            usleep(0x1000);
        in = x86_inb(0x60);
        x86_outb(0x64, 0x60);
        /* Disabling IRQs and enabling clock signal */
        x86_outb(0x60, in & ~((1 << 1) | (1 << 5)));
    }
    
    /* Perform interface tests */
    x86_outb(0x64, 0xAB);
    while (!(x86_inb(0x64) & (1 << 0)))
        usleep(0x1000);
    in = x86_inb(0x60);
    if (in != 0)    return -ENODEV;
    /* Same with second port */
    if (ps2_is_dual)
    {
        x86_outb(0x64, 0xA9);
        while (!(x86_inb(0x64) & (1 << 0)))
            usleep(0x1000);
        in = x86_inb(0x60);
        if (in != 0)    return -ENODEV;
    }

    /* Finally, enable ports */
    x86_outb(0x64, 0xAE);
    if (ps2_is_dual)    x86_outb(0x64, 0xA8);

    /* Enable IRQs back */
    x86_outb(0x64, 0x20);
    while (!(x86_inb(0x64) & (1 << 0)))
        usleep(0x1000);
    in = x86_inb(0x60);
    x86_outb(0x64, 0x60);
    x86_outb(0x60, in | (1 << 0) | (ps2_is_dual ? (1 << 1) : 0));

    /* Reset devices */

    /* First PS/2 port */
    timeout = 128;
    x86_outb(0x64, 0xD2);
    while (x86_inb(0x64) & (1 << 1) && timeout-- > 0)
        usleep(0x1000);
    if (timeout > 0)
    {
        x86_outb(0x60, 0xFF);
        while (!(x86_inb(0x64) & (1 << 0)))
            usleep(0x1000);
        in = x86_inb(0x60);
        if (in != 0xFC)
        {
            /* Initialized PS/2 first port; probe devices */
            ps2_kbd_init(1);
            ps2_mouse_init(1);
        }
    }

    /* Second PS/2 port */
    if (ps2_is_dual)
    {
        timeout = 128;
        x86_outb(0x64, 0xD4);
        while (x86_inb(0x64) & (1 << 1) && timeout-- > 0)
            usleep(0x1000);
        if (timeout > 0)
        {
            x86_outb(0x64, 0xD4);
            x86_outb(0x60, 0xFF);
            while (!(x86_inb(0x64) & (1 << 0)))
                usleep(0x1000);
            in = x86_inb(0x60);
            if (in != 0xFC)
            {
                /* Initialized PS/2 second port; probe devices */
                ps2_kbd_init(2);
                ps2_mouse_init(2);
            }
        }
    }

    /* Flush */
    while (x86_inb(0x64) & (1 << 0)) {
        x86_inb(0x60);
    }

    return 0;
}

static int ps2_probe()
{
    int res;
    if ((res = ps2_init()) != 0)
        return res;

    printk("ps2: %d devices; is_dual = %d", ps2_devices_off, ps2_is_dual);
    irq_request(PS2_KBD_IRQ, &ps2_kbd_irq, NULL);
    irq_request(PS2_MOUSE_IRQ, &ps2_mouse_irq, NULL);
    return 0;
}

static void ps2_cleanup()
{
    while (ps2_devices_off--)
    {
        if (!ps2_devices[ps2_devices_off].port) continue;
        unregister_chardev(ps2_devices[ps2_devices_off].dev);
        ps2_devices[ps2_devices_off].port = 0;
    }

    irq_free(PS2_KBD_IRQ, &ps2_kbd_irq);
    irq_free(PS2_MOUSE_IRQ, &ps2_mouse_irq);
    ps2_devices_off = 0;
}

module_t ps2_module = {
    .probe = ps2_probe,
    .cleanup = ps2_cleanup
};

module_register(
    "ps2",
    ps2_module
);

#endif
#endif
