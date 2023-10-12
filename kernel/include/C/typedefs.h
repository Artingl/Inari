#pragma once

#if !defined(NULL)              /* Define NULL */
	#define NULL ((void*)0)
#endif

// typedef char bool;
// #define false 0
// #define true 1

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef long int off_t;

// typedef enum { false, true } bool;

// #ifdef __INTPTR_TYPE__
// typedef __INTPTR_TYPE__ intptr_t;
// #endif
// #ifdef __UINTPTR_TYPE__
// typedef __UINTPTR_TYPE__ uintptr_t;
// #endif

// typedef long size_t;

// typedef signed char __int8_t;
// typedef unsigned char __uint8_t;
// typedef signed short int __int16_t;
// typedef unsigned short int __uint16_t;
// typedef signed int __int32_t;
// typedef unsigned int __uint32_t;
// typedef signed long long int __int64_t;
// typedef unsigned long long int __uint64_t;

// typedef __uint8_t uint8_t;
// typedef __uint16_t uint16_t;
// typedef __uint32_t uint32_t;
// typedef __uint64_t uint64_t;

// typedef __int8_t int8_t;
// typedef __int16_t int16_t;
// typedef __int32_t int32_t;
// typedef __int64_t int64_t;

// #define CHAR_BIT 8

// /* LIMIT MACROS */
// #define INT8_MIN    (-0x7f - 1)
// #define INT16_MIN   (-0x7fff - 1)
// #define INT32_MIN   (-0x7fffffff - 1)
// #define INT64_MIN   (-0x7fffffffffffffff - 1)

// #define INT8_MAX    0x7f
// #define INT16_MAX   0x7fff
// #define INT32_MAX   0x7fffffff
// #define INT64_MAX   0x7fffffffffffffff

// #define UINT8_MAX   0xff
// #define UINT16_MAX  0xffff
// #define UINT32_MAX  0xffffffff
// #define UINT64_MAX  0xffffffffffffffff

// #define INT_LEAST8_MIN    (-0x7f - 1)
// #define INT_LEAST16_MIN   (-0x7fff - 1)
// #define INT_LEAST32_MIN   (-0x7fffffff - 1)
// #define INT_LEAST64_MIN   (-0x7fffffffffffffff - 1)

// #define INT_LEAST8_MAX    0x7f
// #define INT_LEAST16_MAX   0x7fff
// #define INT_LEAST32_MAX   0x7fffffff
// #define INT_LEAST64_MAX   0x7fffffffffffffff

// #define UINT_LEAST8_MAX   0xff
// #define UINT_LEAST16_MAX  0xffff
// #define UINT_LEAST32_MAX  0xffffffff
// #define UINT_LEAST64_MAX  0xffffffffffffffff

// #define INT_FAST8_MIN     (-0x7f - 1)
// #define INT_FAST16_MIN    (-0x7fff - 1)
// #define INT_FAST32_MIN    (-0x7fffffff - 1)
// #define INT_FAST64_MIN    (-0x7fffffffffffffff - 1)

// #define INT_FAST8_MAX     0x7f
// #define INT_FAST16_MAX    0x7fff
// #define INT_FAST32_MAX    0x7fffffff
// #define INT_FAST64_MAX    0x7fffffffffffffff

// #define UINT_FAST8_MAX    0xff
// #define UINT_FAST16_MAX   0xffff
// #define UINT_FAST32_MAX   0xffffffff
// #define UINT_FAST64_MAX   0xffffffffffffffff

// #define INTPTR_MIN        (-0x7fffffff - 1)
// #define INTPTR_MAX        0x7fffffff
// #define UINTPTR_MAX       0xffffffff

// #define INT8_C(x)    (x)
// #define INT16_C(x)   (x)
// #define INT32_C(x)   ((x) + (INT32_MAX - INT32_MAX))
// #define INT64_C(x)   ((x) + (INT64_MAX - INT64_MAX))

// #define UINT8_C(x)   (x)
// #define UINT16_C(x)  (x)
// #define UINT32_C(x)  ((x) + (UINT32_MAX - UINT32_MAX))
// #define UINT64_C(x)  ((x) + (UINT64_MAX - UINT64_MAX))

// #define INTMAX_C(x)  ((x) + (INT64_MAX - INT64_MAX))
// #define UINTMAX_C(x) ((x) + (UINT64_MAX - UINT64_MAX))

// #define PTRDIFF_MIN  INT32_MIN
// #define PTRDIFF_MAX  INT32_MAX

// #define SIG_ATOMIC_MIN    INT32_MIN
// #define SIG_ATOMIC_MAX    INT32_MAX

// #define SIZE_MAX     UINT32_MAX

// #define WCHAR_MIN    0
// #define WCHAR_MAX    UINT16_MAX

// #define WINT_MIN     0
// #define WINT_MAX     UINT16_MAX

// #define INTMAX_MIN        (-0x7fffffffffffffff - 1)
// #define INTMAX_MAX        0x7fffffffffffffff
// #define UINTMAX_MAX       0xffffffffffffffff
