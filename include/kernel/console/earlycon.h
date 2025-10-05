#ifndef _INARI_EARLY_DEBUG_H
#define _INARI_EARLY_DEBUG_H

#include <misc/types.h>
#include <kernel/inari.h>

typedef struct {
    const char *name;
    int (*probe)(void);
    void (*cleanup)(void);
} earlycon_device_t;

#define earlycon_device(dev_name, probe_fn, cleanup_fn) \
    static const earlycon_device_t __earlycon_dev_##probe_fn \
    __attribute__((used, section(".earlycon"))) = { \
        .name = dev_name, \
        .probe = probe_fn, \
        .cleanup = cleanup_fn \
    }


extern int earlycon_init();
extern void earlycom_cleanup();

#endif
