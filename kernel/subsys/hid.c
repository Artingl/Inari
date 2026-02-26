#ifdef CONFIG_SUBSYS_HID

#include <kernel/inari.h>
#include <kernel/subsys/hid.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/sys/char.h>
#include <kernel/sys/driver.h>
#include <kernel/sys/device.h>
#include <kernel/errno.h>

#include <misc/types.h>
#include <misc/string.h>

static int hid_info(struct hid_device *device, struct hid_device_info *info)
{
    if (!device || !info)    return -EINVAL;
    
    memset(info->name, 0, DEV_NAME_SIZE);
    strcpy(info->name, device->name);
    info->type = device->type;
    return 0;
}

static int hdi_ioctl(struct device *chardev, unsigned long req, void *arg)
{
    if (!chardev || !chardev->driver_data)  return -EINVAL;
    struct hid_device *device = (struct hid_device*)chardev->driver_data;

    switch (req)
    {
    case HID_IOCTL_INFO:
        return hid_info(device, (struct hid_device_info*)arg);
    }

    return -ENOSYS;
}

static int hid_read(struct device *chardev, uint8_t *buf, size_t sz)
{
    /* Buffset size must be exactly kbd_event or mouse_event size */
    if (sz != sizeof(struct kbd_event) && sz != sizeof(struct mouse_event))
        return -EINVAL;
    
    if (!chardev || !chardev->driver_data)  return -EINVAL;
    struct hid_device *device = (struct hid_device*)chardev->driver_data;
    if (!device->ops->read_event)   return -ENOSYS;

    return device->ops->read_event(device, buf);
}

static struct char_ops ops =
{
    .read = &hid_read,
    .ioctl = &hdi_ioctl,
};

int hid_init(void)
{
    register_chardev_group(KBD_DRIVER, "kbd");
    register_chardev_group(MOUSE_DRIVER, "mouse");
    return 0;
}

int hid_add_device(dev_t *dev, uint8_t type, const char *name, struct hid_ops *hid_ops)
{
    if (!hid_ops || !name)    return -EINVAL;

    uint32_t driver;
    if (type == HID_TYPE_KEYBOARD)       driver = KBD_DRIVER;
    else if (type == HID_TYPE_MOUSE)  driver = MOUSE_DRIVER;
    else return -EINVAL;

    int res;
    struct hid_device *device = (struct hid_device*)kmalloc(sizeof(struct hid_device));
    if (!device)  return -ENOMEM;
    
    memset(device->name, 0, DEV_NAME_SIZE);
    strcpy(device->name, name);
    device->ops = hid_ops;
    device->type = type;

    if ((res = register_chardev(driver, &ops, device, &device->dev)) != 0)
    {
        kfree(device);
        return res;
    }

    if (dev) *dev = device->dev;
    return 0;
}

int hid_remove_device(dev_t dev)
{
    struct device *chardev = char_get(dev);
    if (!chardev) return -ENODEV;
    if (chardev->driver_data)   kfree(chardev->driver_data);
    return unregister_chardev(dev);
}

#endif