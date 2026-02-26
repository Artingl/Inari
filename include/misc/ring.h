#ifndef _INARI_MISC_RING_H
#define _INARI_MISC_RING_H

#include <misc/types.h>

struct ring_bbuf {
    uint8_t *buffer;
    int head;
    int tail;
    int maxlen;
};

#define RING_HEAD_INIT(buf, size) ((struct ring_bbuf){ .buffer = (uint8_t*)buf, .maxlen = size, .head = 0, .tail = 0 })
#define RING_HEAD(name, buf, size) \
    struct ring_bbuf name = RING_HEAD_INIT(buf, size)

static inline void ring_bbuf_init(struct ring_bbuf *c, uint8_t *buf, int size)
{
    c->buffer = buf;
    c->maxlen = size;
    c->head = 0;
    c->tail = 0;
}

static inline int ring_bbuf_push(struct ring_bbuf *c, uint8_t data)
{
    int next = (c->head + 1) % c->maxlen; // Calculate next head index

    if (next == c->tail) { // Check if buffer is full
        return -1; // Buffer full, data discarded or error
    }

    c->buffer[c->head] = data; // Store data
    c->head = next;            // Move head pointer
    return 0;
}

static inline int ring_bbuf_pop(struct ring_bbuf *c, uint8_t *data)
{
    if (c->head == c->tail) { // Check if buffer is empty
        return -1;           // Buffer empty, no data to read
    }

    *data = c->buffer[c->tail]; // Read data
    c->tail = (c->tail + 1) % c->maxlen; // Move tail pointer
    return 0;
}

static inline int ring_bbuf_is_empty(struct ring_bbuf *c)
{
    return c->head == c->tail;
}

static inline int ring_bbuf_is_full(struct ring_bbuf *c)
{
    int next = (c->head + 1) % c->maxlen; // Calculate next head index
    return next == c->tail;
}


#endif