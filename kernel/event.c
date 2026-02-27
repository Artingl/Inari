#include <kernel/errno.h>
#include <kernel/event.h>
#include <kernel/inari.h>

#include <misc/string.h>

/* TODO: event queue; event processing in a separate thread */

static size_t handler_offset = 0;
static event_handler_t handlers[CONFIG_MAX_EVENT_SUBSCRIBERS];

int event_bus_init() {
    memset(&handlers, 0, sizeof(handlers));
    return 0;
}

int event_bus_broadcast(event_t event) {
    int res = EVENT_HANDLED;
    for (size_t i = 0; i < handler_offset; i++)
        if (handlers[i]) {
            res = handlers[i](event);
            if (res != EVENT_HANDLED)
                break;
        }
    return res;
}

int event_bus_subscribe(event_handler_t handler) {
    if (handler_offset + 1 >= CONFIG_MAX_EVENT_SUBSCRIBERS)
        return -EINVAL;
    handlers[handler_offset++] = handler;
    return 0;
}
