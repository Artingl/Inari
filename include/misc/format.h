#ifndef _INARI_MISC_FORMAT_H
#define _INARI_MISC_FORMAT_H

#include <misc/types.h>
#include <misc/print.h>
#include <stdarg.h>

struct str_ctx {
    char *buf;
    int pos;
};

static void str_emit(char c, void *userdata) {
    struct str_ctx *ctx = (struct str_ctx *)userdata;
    ctx->buf[ctx->pos++] = c;
}

static inline int vsprintf(char *buf, const char *fmt, va_list args)
{
    struct str_ctx ctx = { .buf = buf, .pos = 0 };
    int count = do_printkn(fmt, args, str_emit, &ctx);
    ctx.buf[ctx.pos] = '\0';
    return count;
}

static inline int sprintf(char *buf, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vsprintf(buf, fmt, args);
    va_end(args);
    return ret;
}
#endif