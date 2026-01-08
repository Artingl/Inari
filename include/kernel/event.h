#ifndef _INARI_EVENT_H
#define _INARI_EVENT_H

#include <misc/types.h>

#include <kernel/sys/driver.h>

#define EVENT_HANDLED   0
#define EVENT_ABORTED   1

typedef enum
{
    EVENT_LOAD_BLKDEV = 0,
    EVENT_UNLOAD_BLKDEV,

    EVENT_LOAD_CHARDEV,
    EVENT_UNLOAD_CHARDEV,

    EVENT_MODULE_LOAD,
    EVENT_MODULE_UNLOAD,

    EVENT_CUSTOM,
} event_type_t;


typedef struct
{
    event_type_t type;

    union {
        dev_t dev;

        void *custom;
    } as;
} event_t;

typedef int(*event_handler_t)(event_t);

typedef struct {

} event_subscriber_t;

extern int event_bus_init();
extern int event_bus_subscribe(event_handler_t handler);
extern int event_bus_broadcast(event_t event);

#endif