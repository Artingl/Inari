#pragma once

#include <kernel/include/C/typedefs.h>

#define SECTOR_SIZE 512

typedef void(*driver_disk_io)(void *device, uint64_t sector, uint64_t n, uint8_t *buffer);

struct driver_disk
{
    void *device;

    driver_disk_io read_handler;
    driver_disk_io write_handler;
};

static struct driver_disk driver_disk_wrap(
    void *device,
    driver_disk_io read_handler,
    driver_disk_io write_handler)
{
    struct driver_disk dev = (struct driver_disk){
        .device = device,
        .read_handler = read_handler,
        .write_handler = write_handler,
    };

    return dev;
}
