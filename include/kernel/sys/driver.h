#ifndef _INARI_DRIVER_H
#define _INARI_DRIVER_H

#include <misc/types.h>

#define DRIVER_SYS_GROUP           0x00
#define DRIVER_DISKS_GROUP         0x01
#define DRIVER_VOLUME_GROUP        0x02
#define DRIVER_INPUT_GROUP         0x03
#define DRIVER_TERMINALS_GROUP     0x04
#define DRIVER_VIDEO_GROUP         0x05
#define DRIVER_NETWORK_GROUP       0x06

#define MKGROUP(driver, group) ((((uint16_t)driver) << 4) | ((uint8_t)group))
#define ISGROUP(driver, group) (((driver) & 0xF) == group)

#define DRVID(dev)         (((dev) >> 16) & 0xFFFF)
#define DEVID(dev)         ((dev) & 0xFFFF)
#define MKDEV(driver, dev) (((driver) << 16) | (dev))

#define UNNAMED_DRIVER		MKGROUP(0, DRIVER_SYS_GROUP)
#define RAMDISK_DRIVER		MKGROUP(1, DRIVER_DISKS_GROUP)
#define ATA_DRIVER          MKGROUP(2, DRIVER_DISKS_GROUP)
#define TTY_DRIVER		    MKGROUP(3, DRIVER_TERMINALS_GROUP)
#define GPT_DRIVER		    MKGROUP(4, DRIVER_VOLUME_GROUP)

#define DRIVER_TOTAL         256

static const char *driver_namings[] =
{
    [ UNNAMED_DRIVER ] = "UNNAMED",
    [ RAMDISK_DRIVER ] = "RAMDISK",
    [ ATA_DRIVER ] = "ATA",
    [ TTY_DRIVER ] = "TTY",
    [ GPT_DRIVER ] = "GPT",

};

typedef unsigned int dev_t;

#endif