#ifndef _LIBC_IO_H
#define _LIBC_IO_H

#include <stdarg.h>
#include <sys.h>

extern handle_t stdio;

int flush(handle_t handle);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif
