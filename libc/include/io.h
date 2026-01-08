#ifndef _LIBC_IO_H
#define _LIBC_IO_H

#include <sys.h>
#include <stdarg.h>

extern handle_t stdout;
extern handle_t stdin;


int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char *buf, const char *fmt, ...);
int printf(const char *fmt, ...);

#endif