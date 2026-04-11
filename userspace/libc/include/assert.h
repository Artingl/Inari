#ifndef _LIBC_ASSERT_H
#define _LIBC_ASSERT_H

#include <io.h>
#include <sys.h>

#include <stdarg.h>

#define assert(c, ...)                                                                                                 \
    do {                                                                                                               \
        int res = 0;                                                                                                   \
        if (!(res = (c))) {                                                                                            \
            printf("assertion failed[%d]: %s: ", res, #c);                                                             \
            printf(__VA_ARGS__);                                                                                       \
            char ch = '\n';                                                                                            \
            flush(stdio);                                                                                              \
            write(stdio, &ch, 1);                                                                                      \
            exit(-1);                                                                                                  \
        }                                                                                                              \
    } while (0)

#endif
