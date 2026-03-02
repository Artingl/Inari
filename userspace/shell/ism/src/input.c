#include <io.h>
#include <sys.h>
#include <string.h>

#include "input.h"
#include "video.h"
#include "core.h"

#define HID_IOCTL_INFO 0 // Device info (provides as hid_device_info structure)

#define HID_TYPE_MOUSE    0
#define HID_TYPE_KEYBOARD 1

struct hid_device_info {
    char name[33];
    uint8_t type;
} __attribute__((packed));

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

static tid_t input_thread = 0;
static handle_t mouse_handle = 0, keyboard_handle = 0;
static struct ism_mouse mouse;
static struct ism_keyboard keyboard;

static void input_handle_devices(void) {
    /* Try to open any first found keyboard and mouse */

    struct fs_node node = {0};
    char path[256] = "/devices/input/";

    while (readdir("/devices/input", &node) > 0) {
        /* Construct path to the device in format /devices/input/DEV_NAME */
        memcpy((void*)&path + 15, node.name, (strlen(node.name) > 200 ? 200 : strlen(node.name)) + 1);

        /* Check if keyboard */
        if (strstr(node.name, "kbd") && !keyboard_handle) {
            open(&keyboard_handle, path, READ);
        }
        /* Maybe mouse? */
        else if (strstr(node.name, "mouse") && !mouse_handle) {
            open(&mouse_handle, path, READ);
        }
    }
}

static void input_thread_entrypoint(void)
{
    struct kbd_event kbd_e;
    struct mouse_event mouse_e;
    struct hid_device_info info;
    int32_t scr_w = 800, scr_h = 600;
    uint32_t last_mouse_event_id;
    handle_t last_mouse_handle = -1, last_keyboard_handle = -1;

    do {
        input_handle_devices();

        /* Print log if device input changed */
        if (last_keyboard_handle != keyboard_handle && keyboard_handle) {
            /* Try to fetch device info */
            if (ioctl(keyboard_handle, HID_IOCTL_INFO, &info) != 0)
                keyboard_handle = 0;
            else {
                printf("%s: new keyboard device: %s.\n", get_name(), info.name);
                last_keyboard_handle = keyboard_handle;
            }
        }
        /* For mouse too */
        if (last_mouse_handle != mouse_handle && mouse_handle) {
            /* Try to fetch device info */
            if (ioctl(mouse_handle, HID_IOCTL_INFO, &info) != 0)
                mouse_handle = 0;
            else {
                printf("%s: new mouse device: %s.\n", get_name(), info.name);
                last_mouse_handle = mouse_handle;
            }
        }

        /* Trick the HID driver to not block our read by resetting event_id */
        kbd_e.event_id = -1;
        mouse_e.event_id = -1;

        /* Read most recent info from devices */
        if (read(mouse_handle, (void*)&mouse_e, sizeof(struct mouse_event), NULL) != 0)
            mouse_handle = 0;
        else if (last_mouse_event_id != mouse_e.event_id) {
            last_mouse_event_id = mouse_e.event_id;
            memcpy((void*)mouse.buttons, (void*)mouse_e.buttons, sizeof(mouse.buttons));
            mouse.pos_x += mouse_e.rel_x;
            mouse.pos_y += mouse_e.rel_y;
            mouse.rel_x = mouse_e.rel_x;
            mouse.rel_y = mouse_e.rel_y;

            /* Limit mouse position to screen boundaries */
            ism_video_resolution(&scr_w, &scr_h, NULL);
            if (mouse.pos_x < 0) mouse.pos_x = 0;
            if (mouse.pos_y < 0) mouse.pos_y = 0;
            if (mouse.pos_x > (int16_t)scr_w - 1) mouse.pos_x = (int16_t)scr_w - 1;
            if (mouse.pos_y > (int16_t)scr_h - 1) mouse.pos_y = (int16_t)scr_h - 1;

            mouse.wheel_rel_x = mouse_e.wheel_rel_x;
            mouse.wheel_rel_y = mouse_e.wheel_rel_y;
        }

        if (read(keyboard_handle, (void*)&kbd_e, sizeof(struct kbd_event), NULL) != 0)
            keyboard_handle = 0;
        else {
            keyboard.released = kbd_e.released;
            keyboard.code = kbd_e.code;
            keyboard.key = kbd_e.key;
        }

    } while (is_running());
}

int input_read(struct ism_mouse *l_mouse, struct ism_keyboard *l_keyboard)
{
    if (l_mouse)    memcpy((void*)l_mouse, (void*)&mouse, sizeof(struct ism_mouse));
    if (l_keyboard) memcpy((void*)l_keyboard, (void*)&keyboard, sizeof(struct ism_keyboard));
    return 0;
}

int input_init(void)
{
    pid_t pid;
    if (get_pid(&pid) != 0) return -1;

    return spawn_thread(&input_thread, pid, &input_thread_entrypoint);
}

int input_cleanup(void)
{
    if (!input_thread) return -1;
    return kill_thread(input_thread);
}
