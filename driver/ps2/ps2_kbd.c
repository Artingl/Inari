#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_PS2_KBD

#include <kernel/inari.h>
#include <kernel/console/console.h>
#include <kernel/errno.h>
#include <kernel/module.h>
#include <kernel/interrupts/irq.h>

#include <misc/string.h>
#include <arch/x86/arch.h>

#define PS2_KBD_IRQ 0x1

static int ps2_kb_initialized = 0;
static const char *ps2_kbd_type;

/* special keycodes start above ASCII range */
#define KEY_NONE       0x000
#define KEY_ESC        0x100
#define KEY_ENTER      0x101
#define KEY_BACKSPACE  0x102
#define KEY_TAB        0x103
#define KEY_LCTRL      0x104
#define KEY_RCTRL      0x105
#define KEY_LSHIFT     0x106
#define KEY_RSHIFT     0x107
#define KEY_LALT       0x108
#define KEY_RALT       0x109
#define KEY_CAPSLOCK   0x10A
#define KEY_NUMLOCK    0x10B
#define KEY_SCROLLLOCK 0x10C
#define KEY_F1         0x120
#define KEY_F2         0x121
#define KEY_F3         0x122
#define KEY_F4         0x123
#define KEY_F5         0x124
#define KEY_F6         0x125
#define KEY_F7         0x126
#define KEY_F8         0x127
#define KEY_F9         0x128
#define KEY_F10        0x129
#define KEY_F11        0x12A
#define KEY_F12        0x12B

/* keypad */
#define KEY_KP_STAR    0x130
#define KEY_KP_MINUS   0x131
#define KEY_KP_PLUS    0x132
#define KEY_KP_ENTER   0x133
#define KEY_KP_SLASH   0x134

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
    [0x47] = '7',   /* keypad 7 (Home) */
    [0x48] = '8',   /* keypad 8 (Up) */
    [0x49] = '9',   /* keypad 9 (PgUp) */
    [0x4A] = KEY_KP_MINUS,
    [0x4B] = '4',   /* keypad 4 (Left) */
    [0x4C] = '5',   /* keypad 5 */
    [0x4D] = '6',   /* keypad 6 (Right) */
    [0x4E] = KEY_KP_PLUS,
    [0x4F] = '1',   /* keypad 1 (End) */
    [0x50] = '2',   /* keypad 2 (Down) */
    [0x51] = '3',   /* keypad 3 (PgDn) */
    [0x52] = '0',   /* keypad 0 (Ins) */
    [0x53] = '.',   /* keypad . (Del) */
    [0x54] = KEY_NONE,
    [0x55] = KEY_NONE,
    [0x56] = KEY_NONE,
    [0x57] = KEY_F11,
    [0x58] = KEY_F12,
    /* 0x59–0x7F: unused or handled as extended */
};

static int ps2_kbd_irq(uint32_t irq, void *dev_id)
{
    uint8_t in = x86_inb(0x60);
    int released = in & 0x80;
    uint8_t code = in & 0x7F;
    uint16_t key = scancode_set1_map[code];
    if (!released)
        console_printc(0, (const char*)&key, 1);
    x86_outb(0x64, in);
    return IRQ_HANDLED;
}

int ps2_kbd_probe()
{
    uint8_t in, b0, b1;

    /* Check that we have PS/2 keyboard */
    x86_outb(0x60, 0xEE);
    while ((in = x86_inb(0x60)) == 0xFE)
        usdelay(0x1000);
    if (in != 0xEE) {
        printk("ps2: keyboard not found");
        return -ENODEV;
    }

    /* Identify keyboard */
    x86_outb(0x60, 0xF5);
    while ((in = x86_inb(0x60)) == 0xFE) {
        usdelay(0x1000);
        x86_outb(0x60, 0xF5);
    }
    x86_outb(0x60, 0xF2);
    while ((in = x86_inb(0x60)) == 0xFE) {
        usdelay(0x1000);
        x86_outb(0x60, 0xF2);
    }
    b0 = x86_inb(0x60);
    usdelay(0x1000);
    b1 = x86_inb(0x60);
    usdelay(0x1000);
    x86_outb(0x60, 0xF4);
    while ((in = x86_inb(0x60)) == 0xFE) {
        usdelay(0x1000);
        x86_outb(0x60, 0xF4);
    }

    if ((b0 == 0xAB && b1 == 0x41) || (b0 == 0xAB && b1 == 0xC1)) ps2_kbd_type = "MF2 keyboard";
    else if (b0 == 0xAB && b1 == 0x54) ps2_kbd_type = "IBM ThinkPad";
    else if (b0 == 0xAB && b1 == 0x85) ps2_kbd_type = "NCD N-97 keyboard";
    else if (b0 == 0xAB && b1 == 0x86) ps2_kbd_type = "122-key keyboard";
    else if (b0 == 0xAB && b1 == 0x90) ps2_kbd_type = "Japanese \"G\" keyboard";
    else if (b0 == 0xAB && b1 == 0x91) ps2_kbd_type = "Japanese \"P\" keyboard";
    else if (b0 == 0xAB && b1 == 0x92) ps2_kbd_type = "Japanese \"A\" keyboard";
    else if (b0 == 0xAC && b1 == 0xA1) ps2_kbd_type = "NCD Sun layout keyboard ";
    else ps2_kbd_type = "unkown";

    printk("ps2: keyboard type is %s", ps2_kbd_type);
    irq_request(PS2_KBD_IRQ, &ps2_kbd_irq, NULL);

    ps2_kb_initialized = 1;
    return 0;
}

void ps2_kbd_cleanup()
{
}

module_register(
    "ps2_kbd",
    ps2_kbd_probe,
    ps2_kbd_cleanup
);

#endif
#endif
