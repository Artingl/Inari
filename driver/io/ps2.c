#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_PS2
#ifdef CONFIG_SUBSYS_HID

#include <kernel/console/console.h>
#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/interrupts/irq.h>
#include <kernel/module.h>
#include <kernel/proc/sched.h>
#include <kernel/subsys/hid.h>
#include <kernel/sys/device.h>
#include <kernel/timer.h>

#include <arch/sys.h>
#include <arch/x86/arch.h>
#include <misc/string.h>

#define PS2_IRQ1  1
#define PS2_IRQ12 12

#define PS2_TYPE_UNKNOWN     -1
#define PS2_TYPE_MOUSE_STD   0
#define PS2_TYPE_MOUSE_WHEEL 1
#define PS2_TYPE_MOUSE_5BTN  2
#define PS2_TYPE_KBD_STD     3
#define PS2_TYPE_KBD_ANCIENT 4

static int ps2_is_dual = 0;

static size_t ps2_devices_off = 0;
static struct {
    uint8_t port; // 0 - disabled; 1 - first; 2 - second
    uint8_t type; // 1 - keyboard; 2 - mouse
    uint8_t dev_type;
    dev_t dev;

    uint8_t mouse_state_idx;
    uint8_t mouse_state[3];

    struct kbd_event kbd;
    struct mouse_event mouse;
} ps2_devices[2] = {0};

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

static void ps2_wait_write() {
    int timeout = 1000;
    while ((x86_inb(0x64) & 2) && timeout-- > 0) {
        usleep(1000);
    }
}

static int ps2_send_command(uint8_t port, uint8_t cmd) {
    int retries = 3;
    int timeout;
    uint8_t response;

    while (retries-- > 0) {
        if (port == 2) {
            ps2_wait_write();
            x86_outb(0x64, 0xD4);
        }

        ps2_wait_write();
        x86_outb(0x60, cmd);

        timeout = 100;
        while (!(x86_inb(0x64) & 1) && timeout-- > 0) {
            usleep(1000);
        }

        if (timeout <= 0) {
            kprintf("ps2: cmd 0x%x: timeout!", cmd);
            return -1;
        }

        response = x86_inb(0x60);

        if (response == 0xFA) {
            return 0;
        } else if (response == 0xFE) {
            continue;
        } else {
            kprintf("ps2: cmd 0x%x: failed with unexpected byte 0x%x", cmd, response);
            return -1;
        }
    }
    return -1;
}

static int ps2_identify(uint8_t port) {
    uint8_t b0 = 0xFF, b1 = 0xFF;
    int timeout;

    /* Disable Scanning */
    if (ps2_send_command(port, 0xF5) != 0) {
        return PS2_TYPE_UNKNOWN;
    }

    /* Identify Command */
    if (ps2_send_command(port, 0xF2) != 0) {
        return PS2_TYPE_UNKNOWN;
    }

    /* Read first ID byte with timeout */
    timeout = 100;
    while (!(x86_inb(0x64) & 1) && timeout-- > 0)
        usleep(1000);

    if (timeout > 0) {
        b0 = x86_inb(0x60);

        /* Try to read a second ID byte with timeout */
        timeout = 100;
        while (!(x86_inb(0x64) & 1) && timeout-- > 0)
            usleep(1000);

        if (timeout > 0) {
            b1 = x86_inb(0x60);
        }
    } else {
        kprintf("ps2: unknown device id 0x%x 0x%x", b0, b1);
        return PS2_TYPE_UNKNOWN;
    }

    /* Enable Data Reporting */
    ps2_send_command(port, 0xF4);

    /* Determine Device Type */
    if (b0 == 0x00)
        return PS2_TYPE_MOUSE_STD;
    if (b0 == 0x03)
        return PS2_TYPE_MOUSE_WHEEL;
    if (b0 == 0x04)
        return PS2_TYPE_MOUSE_5BTN;
    if (b0 == 0xAB && (b1 == 0x83 || b1 == 0x41 || b1 == 0xC1))
        return PS2_TYPE_KBD_STD;

    if (b0 == 0xFF)
        return PS2_TYPE_KBD_ANCIENT;

    kprintf("ps2: unknown device id 0x%x 0x%x", b0, b1);
    return PS2_TYPE_UNKNOWN;
}

static void ps2_handle_input() {
    size_t i;
    uint8_t in, status;
    int8_t rel;

    while ((status = x86_inb(0x64)) & 1) {
        in = x86_inb(0x60);

        for (i = 0; i < ps2_devices_off; i++) {
            if (ps2_devices[i].port != (status & 0x20 ? 2 : 1))
                continue;

            /* Check if mouse; Type 2 == mouse */
            if (ps2_devices[i].type == 2) {
                switch (ps2_devices[i].mouse_state_idx++) {
                case 0:
                    ps2_devices[i].mouse_state[0] = in;

                    /* Sync packets */
                    if (!(ps2_devices[i].mouse_state[0] & 0x08)) {
                        ps2_devices[i].mouse_state_idx = 0;
                        break;
                    }

                    ps2_devices[i].mouse.buttons[HID_MOUSE_BTN1] = in & 1;
                    ps2_devices[i].mouse.buttons[HID_MOUSE_BTN2] = in & 2;
                    ps2_devices[i].mouse.buttons[HID_MOUSE_BTN3] = in & 4;
                    break;

                case 1: // X movement
                    if (ps2_devices[i].mouse_state[0] & 0x80 || ps2_devices[i].mouse_state[0] & 0x40)
                        break;
                    rel = in;
                    if (ps2_devices[i].mouse_state[0] >> 4)
                        rel -= 256;
                    ps2_devices[i].mouse.rel_x = rel;
                    break;

                case 2: // Y movement
                    if (ps2_devices[i].mouse_state[0] & 0x80 || ps2_devices[i].mouse_state[0] & 0x40)
                        break;
                    rel = in;
                    if (ps2_devices[i].mouse_state[0] >> 5)
                        rel -= 256;
                    ps2_devices[i].mouse.rel_y = rel * -1;

                    if (ps2_devices[i].dev_type == PS2_TYPE_MOUSE_STD) {
                        ps2_devices[i].mouse_state_idx = 0; /* Otherwise, the mouse sends 4 packet bytes */
                        ps2_devices[i].mouse.event_id++;
                    }
                    break;

                case 3:
                    ps2_devices[i].mouse_state_idx = 0;
                    ps2_devices[i].mouse.event_id++;
                    break;
                }
                break;
            }
            /* Otherwise keyboard; Type 1 == keyboard */
            else if (ps2_devices[i].type == 1) {

                ps2_devices[i].kbd.event_id++;
                ps2_devices[i].kbd.released = in & 0x80;
                ps2_devices[i].kbd.code = in & 0x7F;
                ps2_devices[i].kbd.key = scancode_set1_map[ps2_devices[i].kbd.code];
                break;
            }
        }
    }
}

static int ps2_irq1(uint32_t irq, void *driver_data) {
    ps2_handle_input();
    return IRQ_HANDLED;
}

static int ps2_irq12(uint32_t irq, void *driver_data) {
    ps2_handle_input();
    return IRQ_HANDLED;
}

static int ps2_read_event(struct hid_device *device, void *event) {
    size_t dev_id = 0;
    if (!device || !event)
        return -EINVAL;

    for (dev_id = 0; dev_id < ps2_devices_off; dev_id++) {
        if (ps2_devices[dev_id].port == 0)
            continue;

        /* Type 1 == keyboard; Type 2 == mouse */
        if (device->type == HID_TYPE_KEYBOARD && ps2_devices[dev_id].type == 1) {
            memcpy(event, (void *)&ps2_devices[dev_id].kbd, sizeof(struct kbd_event));
            return 0;
        }
        if (device->type == HID_TYPE_MOUSE && ps2_devices[dev_id].type == 2) {
            memcpy(event, (void *)&ps2_devices[dev_id].mouse, sizeof(struct mouse_event));
            return 0;
        }
    }

    return -ENODEV;
}

struct hid_ops ps2_hid_ops = {.read_event = &ps2_read_event};

static int ps2_kbd_init(uint8_t port) {
    int type, timeout;
    dev_t dev;
    if (ps2_devices_off >= 2)
        return -ENODEV;

    /* Identify keyboard */
    type = ps2_identify(port);
    if (type != PS2_TYPE_KBD_STD && type != PS2_TYPE_KBD_ANCIENT)
        return -ENODEV;

    /* Check that we have PS/2 keyboard */
    if (port == 2)
        x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xEE);
    timeout = 100;
    while (!(x86_inb(0x64) & 1) && timeout-- > 0)
        usleep(1000);
    if (timeout <= 0 || x86_inb(0x60) != 0xEE)
        return -ENODEV;

    if (hid_add_device(&dev, HID_TYPE_KEYBOARD, "PS/2 Keyboard", &ps2_hid_ops) != 0)
        return -ENOSYS;

    ps2_devices[ps2_devices_off].dev = dev;
    ps2_devices[ps2_devices_off].port = port;
    ps2_devices[ps2_devices_off].dev_type = type;
    ps2_devices[ps2_devices_off++].type = 1;
    kprintf("ps2: initalized keyboard on port %d; type %d", port, type);
    return 0;
}

static int ps2_mouse_init(uint8_t port) {
    int type, timeout;
    dev_t dev;
    if (ps2_devices_off >= 2)
        return -ENODEV;

    /* Check that we have PS/2 mouse */
    if (port == 2)
        x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xFF);
    timeout = 100;
    while (!(x86_inb(0x64) & 1) && timeout-- > 0)
        usleep(1000);
    if (timeout <= 0 || x86_inb(0x60) != 0xFA)
        return -ENODEV;
    timeout = 1000;
    while (!(x86_inb(0x64) & 1) && timeout-- > 0)
        usleep(1000);
    if (timeout <= 0 || x86_inb(0x60) != 0xAA)
        return -ENODEV;
    timeout = 100;
    while (!(x86_inb(0x64) & 1) && timeout-- > 0)
        usleep(1000);
    if (timeout <= 0)
        return -ENODEV;
    x86_inb(0x60);

    /* Identify mouse */
    type = ps2_identify(port);
    if (type != PS2_TYPE_MOUSE_STD && type != PS2_TYPE_MOUSE_WHEEL && type != PS2_TYPE_MOUSE_5BTN)
        return -ENODEV;

    /* Set sample rate */
    // ps2_send_command(port, 0xf3);
    // ps2_send_command(port, 200);

    if (port == 2)
        x86_outb(0x64, 0xD4);
    x86_outb(0x60, 0xF4);
    timeout = 100;
    while (!(x86_inb(0x64) & 1) && timeout-- > 0)
        usleep(1000);
    if (timeout <= 0 || x86_inb(0x60) != 0xFA)
        return -ENODEV;
    if (hid_add_device(&dev, HID_TYPE_MOUSE, "PS/2 Mouse", &ps2_hid_ops) != 0)
        return -ENOSYS;

    ps2_devices[ps2_devices_off].dev = dev;
    ps2_devices[ps2_devices_off].port = port;
    ps2_devices[ps2_devices_off].dev_type = type;
    ps2_devices[ps2_devices_off++].type = 2;
    kprintf("ps2: initalized mouse on port %d; type %d", port, type);
    return 0;
}

static int ps2_init() {
    uint8_t in;
    ps2_is_dual = 0;

    /* Disable devices */
    x86_outb(0x64, 0xAD);
    x86_outb(0x64, 0xA7);

    /* Flush the output buffer */
    x86_inb(0x60);

    /* Set the controller configuration byte */
    x86_outb(0x64, 0x20);
    while (!(x86_inb(0x64) & 1))
        usleep(0x1000);
    in = x86_inb(0x60);
    x86_outb(0x64, 0x60);
    /* Disabling IRQs for port 1; Enabling clock signal */
    x86_outb(0x60, in & ~(1 | (1 << 4)));

    /* Perform Controller Self Test */
    x86_outb(0x64, 0xAA);
    while (!(x86_inb(0x64) & 1))
        usleep(0x1000);
    in = x86_inb(0x60);
    if (in != 0x55) {
        kprintf("ps2: self-test failed.");
        return -ENODEV;
    }

    /* Determine if there are 2 channels */
    x86_outb(0x64, 0xA8);
    x86_outb(0x64, 0x20);
    while (!(x86_inb(0x64) & 1))
        usleep(0x1000);
    in = x86_inb(0x60);
    if ((ps2_is_dual = !(in & (1 << 5)))) {
        /* Second port available. Disable again */
        x86_outb(0x64, 0xA7);

        x86_outb(0x64, 0x20);
        while (!(x86_inb(0x64) & 1))
            usleep(0x1000);
        in = x86_inb(0x60);
        x86_outb(0x64, 0x60);
        /* Disabling IRQs and enabling clock signal */
        x86_outb(0x60, in & ~((1 << 1) | (1 << 5)));
    }

    /* Perform interface tests */
    x86_outb(0x64, 0xAB);
    while (!(x86_inb(0x64) & 1))
        usleep(0x1000);
    in = x86_inb(0x60);
    if (in != 0)
        return -ENODEV;
    /* Same with second port */
    if (ps2_is_dual) {
        x86_outb(0x64, 0xA9);
        while (!(x86_inb(0x64) & 1))
            usleep(0x1000);
        in = x86_inb(0x60);
        if (in != 0)
            return -ENODEV;
    }

    /* Finally, enable ports */
    x86_outb(0x64, 0xAE);
    if (ps2_is_dual)
        x86_outb(0x64, 0xA8);

    /* Initialize devices */
    if (ps2_kbd_init(1) != 0)
        ps2_mouse_init(1);

    if (ps2_is_dual) {
        if (ps2_kbd_init(2) != 0)
            ps2_mouse_init(2);
    }

    /* Flush */
    while (x86_inb(0x64) & 1) {
        x86_inb(0x60);
    }

    /* Enable IRQs back */
    x86_outb(0x64, 0x20);
    while (!(x86_inb(0x64) & 1))
        usleep(0x1000);
    in = x86_inb(0x60);

    ps2_wait_write();
    x86_outb(0x64, 0x60);
    ps2_wait_write();
    x86_outb(0x60, in | 1 | (ps2_is_dual ? (1 << 1) : 0));

    return 0;
}

static int ps2_probe() {
    int res;
    if ((res = ps2_init()) != 0)
        return res;

    kprintf("ps2: %d devices; is_dual = %d", ps2_devices_off, ps2_is_dual);
    irq_request(PS2_IRQ1, &ps2_irq1, NULL);
    irq_request(PS2_IRQ12, &ps2_irq12, NULL);
    return 0;
}

static void ps2_cleanup() {
    size_t i;
    for (i = 0; i < ps2_devices_off; i++) {
        if (ps2_devices[i].port == 0)
            continue;
        hid_remove_device(ps2_devices[i].dev);
        ps2_devices[i].port = 0;
    }

    irq_free(PS2_IRQ1, &ps2_irq1);
    irq_free(PS2_IRQ12, &ps2_irq12);
    ps2_devices_off = 0;
}

module_t ps2_module = {.probe = ps2_probe, .cleanup = ps2_cleanup};

module_register("ps2", ps2_module);

#endif
#endif
#endif
