#pragma once

#include "kernel/libc/typedefs.h"

#include <stdarg.h>

int do_printkn(const char *fmt, va_list args, int (*fn)(char, void **), void *ptr);
