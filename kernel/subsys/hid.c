#ifdef CONFIG_SUBSYS_HID

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/proc/sched.h>
#include <kernel/subsys/hid.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>
#include <kernel/subsys/video.h>

#include <misc/string.h>
#include <misc/types.h>

static int hid_info(struct hid_device *device, struct hid_device_info *info) {
    if (!device || !info)
        return -EINVAL;

    memset(info->name, 0, DEV_NAME_SIZE);
    strcpy(info->name, device->name);
    info->type = device->type;
    return 0;
}

static int hdi_ioctl(struct device *chardev, unsigned long req, void *arg) {
    if (!chardev || !chardev->driver_data)
        return -EINVAL;
    struct hid_device *device = (struct hid_device *)chardev->driver_data;

    switch (req) {
    case HID_IOCTL_INFO:
        return hid_info(device, (struct hid_device_info *)arg);
    }

    return -ENOSYS;
}

static int hid_read(struct device *chardev, uint8_t *buf, size_t sz) {
    /* Buffset size must be exactly kbd_event or mouse_event size */
    if (sz != sizeof(struct kbd_event) && sz != sizeof(struct mouse_event))
        return -EINVAL;

    if (!chardev || !chardev->driver_data)
        return -EINVAL;
    struct hid_device *device = (struct hid_device *)chardev->driver_data;
    struct kbd_event *kbd = (struct kbd_event *)buf;
    struct mouse_event *mouse = (struct mouse_event *)buf;
    int res = 0;
    if (!device->ops->read_event)
        return -ENOSYS;

    uint32_t prev_event_id = device->type == HID_TYPE_KEYBOARD ? kbd->event_id : mouse->event_id;
    uint32_t event_id;

    /* Ensure we're not sending outdated info */
    while ((res = device->ops->read_event(device, buf)) == 0) {
        event_id = device->type == HID_TYPE_KEYBOARD ? kbd->event_id : mouse->event_id;

        /* If we got same event, skip */
        if (event_id == prev_event_id) {
            sched_yield();
            continue;
        }

        break;
    }

    return res;
}

static tid_t console_thread;
static void hid_console_thread(void *_) {
    do {
        /* TODO: semaphore */
        if (video_state() != 0) {
            /* Currently not in text mode */
            timer_usleep(1000000);
        }

        // console_write_queue()

        timer_usleep(20000);
    } while(1);
}

static struct char_ops ops = {
    .read = &hid_read,
    .ioctl = &hdi_ioctl,
};

int hid_init(void) {
    register_chardev_group(KBD_DRIVER, "kbd");
    register_chardev_group(MOUSE_DRIVER, "mouse");
    return sched_create_thread("hid_console_io", &console_thread, &hid_console_thread, NULL, NULL, NULL);
}

int hid_add_device(dev_t *dev, uint8_t type, const char *name, struct hid_ops *hid_ops) {
    if (!hid_ops || !name)
        return -EINVAL;

    uint32_t driver;
    if (type == HID_TYPE_KEYBOARD)
        driver = KBD_DRIVER;
    else if (type == HID_TYPE_MOUSE)
        driver = MOUSE_DRIVER;
    else
        return -EINVAL;

    int res;
    struct hid_device *device = (struct hid_device *)kmalloc(sizeof(struct hid_device));
    if (!device)
        return -ENOMEM;

    memset(device->name, 0, DEV_NAME_SIZE);
    strcpy(device->name, name);
    device->ops = hid_ops;
    device->type = type;

    if ((res = register_chardev(driver, &ops, device, &device->dev)) != 0) {
        kfree(device);
        return res;
    }

    if (dev)
        *dev = device->dev;
    return 0;
}

int hid_remove_device(dev_t dev) {
    struct device *chardev = char_get(dev);
    if (!chardev)
        return -ENODEV;
    if (chardev->driver_data)
        kfree(chardev->driver_data);
    return unregister_chardev(dev);
}

#endif
