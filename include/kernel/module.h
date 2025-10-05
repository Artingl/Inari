#ifndef _INARI_MODULE_H
#define _INARI_MODULE_H

typedef struct {
    const char *name;
    int (*probe)(void);
    void (*cleanup)(void);
} module_t;

#define module_register(module_name, probe_fn, cleanup_fn) \
    static const module_t __module_##probe_fn \
    __attribute__((used, section(".modules"))) = { \
        .name = module_name, \
        .probe = probe_fn, \
        .cleanup = cleanup_fn \
    }


extern int modules_init();

#endif