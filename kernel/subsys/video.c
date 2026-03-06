#ifdef CONFIG_SUBSYS_VIDEO

#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>
#include <kernel/mm/vmm.h>
#include <kernel/subsys/video.h>
#include <kernel/sys/char.h>
#include <kernel/sys/device.h>
#include <kernel/sys/driver.h>

#include <misc/list.h>
#include <misc/string.h>
#include <misc/types.h>

static LIST_HEAD(video_devices);
static int is_initialized = 0;

static int video_ioctl(struct device *chardev, unsigned long req, void *arg) {
    if (!chardev || !chardev->driver_data)
        return -EINVAL;
    struct video_device *device = (struct video_device *)chardev->driver_data;

    switch (req) {
    case VIDEO_IOCTL_INFO:
        if (device->ops->mode_info && VMM_IS_PTR_USERSPACE(arg))
            return device->ops->mode_info(device, arg);
        break;
    case VIDEO_IOCTL_MODE_SWITCH:
        if (device->ops->mode_switch && VMM_IS_PTR_USERSPACE(arg))
            return device->ops->mode_switch(device, arg);
        break;
    case VIDEO_IOCTL_MODE_FIND_NEXT:
        if (device->ops->mode_find_next && VMM_IS_PTR_USERSPACE(arg))
            return device->ops->mode_find_next(device, arg);
        break;
    case VIDEO_IOCTL_BLIT:
        if (device->ops->blit && VMM_IS_PTR_USERSPACE(arg))
            return device->ops->blit(device, arg);
        break;
    case VIDEO_IOCTL_DISABLE:
        if (device->ops->disable) {
            device->ops->disable(device);
            return 0;
        }
        break;
    case VIDEO_IOCTL_FILL_RECT:
        if (device->ops->fill_rect && VMM_IS_PTR_USERSPACE(arg))
            return device->ops->fill_rect(device, arg);
        break;
    case VIDEO_IOCTL_MAP:
        if (device->ops->map_video && VMM_IS_PTR_USERSPACE(arg))
            return device->ops->map_video(device, arg);
        break;
    }

    return -ENOSYS;
}

static struct char_ops ops = {.ioctl = &video_ioctl};

int video_init(void) {
    register_chardev_group(VIDEO_DRIVER, "video");
    is_initialized = 1;
    return 0;
}

int video_add_device(dev_t *dev, const char *name, uintptr_t base, struct video_ops *video_ops) {
    if (!video_ops || !name || !is_initialized)
        return -EINVAL;

    int res;
    struct video_device *device = (struct video_device *)kmalloc(sizeof(struct video_device));
    if (!device)
        return -ENOMEM;

    strcpy(device->name, name);
    device->name[DEV_NAME_SIZE] = '\0';
    device->base = base;
    device->ops = video_ops;

    if ((res = register_chardev(VIDEO_DRIVER, &ops, device, &device->dev)) != 0) {
        kfree(device);
        return res;
    }

    list_add(&device->list, &video_devices);
    if (dev)
        *dev = device->dev;

    return 0;
}

int video_remove_device(dev_t dev) {
    if (!is_initialized)
        return -EINVAL;

    struct device *chardev = char_get(dev);
    if (!chardev)
        return -ENODEV;
    if (chardev->driver_data) {
        list_del(&((struct video_device *)chardev->driver_data)->list);
        kfree(chardev->driver_data);
    }
    return unregister_chardev(dev);
}

int video_disable(void) {
    if (!is_initialized)
        return -EINVAL;

    struct video_device *entry;
    struct list_head *pos;

    list_for_each(pos, &video_devices) {
        entry = list_entry(pos, struct video_device, list);
        if (entry->ops && entry->ops->disable)
            entry->ops->disable(entry);
    }

    return 0;
}

struct video_device *video_get(dev_t dev) {
    if (!is_initialized || !ISGROUP(DRVID(dev), DRIVER_VIDEO_GROUP))
        return NULL;
    struct device *chardev = char_get(dev);
    if (!chardev)
        return NULL;
    return chardev->driver_data;
}

#endif
