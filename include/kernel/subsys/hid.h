#ifndef _INARI_KBD_H
#define _INARI_KBD_H

#include <kernel/sys/device.h>
#include <misc/types.h>

#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

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
#define KEY_KP_STAR  0x130
#define KEY_KP_MINUS 0x131
#define KEY_KP_PLUS  0x132
#define KEY_KP_ENTER 0x133
#define KEY_KP_SLASH 0x134
#define KEY_KP_HOME  0x135
#define KEY_KP_UP    0x136
#define KEY_KP_PGUP  0x137
#define KEY_KP_LEFT  0x138
#define KEY_KP_RIGHT 0x139
#define KEY_KP_END   0x13a
#define KEY_KP_DOWN  0x13b
#define KEY_KP_PGDN  0x13c
#define KEY_KP_INS   0x13d
#define KEY_KP_DEL   0x13e

struct hid_device;

/* dev - the keyboard device
 * released - was the keyboard key released
 * code - the keycode (driver specific)
 * key - a key scancode */
struct kbd_event {
    int released;
    uint8_t code;
    uint16_t key;

    /* Internal stuff. Used by driver to check if we're not duplicating events */
    uint32_t event_id;
} __attribute__((packed));

struct mouse_event {
#define HID_MOUSE_BTN1 0 // Left button
#define HID_MOUSE_BTN2 1 // Right button
#define HID_MOUSE_BTN3 2 // Middle button
    uint8_t buttons[8];  // Are any buttons pressed?

    int16_t rel_x;
    int16_t rel_y;

    int16_t wheel_rel_x;
    int16_t wheel_rel_y;

    /* Internal stuff. Used by driver to check if we're not duplicating events */
    uint32_t event_id;
} __attribute__((packed));

struct hid_ops {
    /* Reads latest device event and puts the appropriate structure
       (e.g. kbd_event or mouse_event based on device type) into event argument */
    int (*read_event)(struct hid_device *device, void *event);
} __attribute__((packed));

struct hid_device_info {
    char name[DEV_NAME_SIZE + 1];
    uint8_t type;
} __attribute__((packed));

struct hid_device {
    char name[DEV_NAME_SIZE + 1];
    struct hid_ops *ops;
    uint8_t type;
    dev_t dev;
} __attribute__((packed));

#define HID_IOCTL_INFO 0 // Device info (provides as hid_device_info structure)

#define HID_TYPE_MOUSE    0
#define HID_TYPE_KEYBOARD 1

int hid_init(void);
int hid_add_device(dev_t *dev, uint8_t type, const char *name, struct hid_ops *ops);
int hid_remove_device(dev_t dev);

#endif