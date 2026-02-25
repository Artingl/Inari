#ifndef _INARI_KBD_H
#define _INARI_KBD_H

#include <kernel/sys/device.h>
#include <misc/types.h>

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
#define KEY_KP_HOME    0x135
#define KEY_KP_UP      0x136
#define KEY_KP_PGUP    0x137
#define KEY_KP_LEFT    0x138
#define KEY_KP_RIGHT   0x139
#define KEY_KP_END     0x13a
#define KEY_KP_DOWN    0x13b
#define KEY_KP_PGDN    0x13c
#define KEY_KP_INS     0x13d
#define KEY_KP_DEL     0x13e

/* Each keyboard chardev must return this structure on read.
 * This structure shows the latest state of the keyboard device.
 *
 * event_id - increments on each new keyboard event.
 *            useful to check whether new events happened or not.
 *            if 0, means no events happened since driver startup.
 * dev - the keyboard device
 * released - was the keyboard key released
 * code - the keycode (driver specific)
 * key - a key scancode */
struct kbd_event
{
    uint32_t event_id;
    dev_t dev;

    int released;
    uint8_t code;
    uint16_t key;
};

#endif