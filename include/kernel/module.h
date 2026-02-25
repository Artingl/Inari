#ifndef _INARI_MODULE_H
#define _INARI_MODULE_H

#include "kernel/event.h"

#define MODULE_LAZY_LOAD    (1 << 0)

typedef struct {
    int (*probe)(void);
    void (*cleanup)(void);
    event_handler_t event_bus;

    uint8_t flags;
    uint8_t is_loaded;
} module_t;

typedef struct {
    const char *name;
    module_t *module;
} module_metadata_t;

#define module_register(module_name, module_meta) \
    static const module_metadata_t __module_##probe_fn \
    __attribute__((used, section(".modules"))) = { \
        .name = module_name, \
        .module = &module_meta, \
    }


void modules_cleanup();
int modules_ls(int idx, char *name, uintptr_t *ptr, uint32_t *state);
int modules_insmod(const char *name);
int modules_rmmod(const char *name);
int modules_init();

#endif