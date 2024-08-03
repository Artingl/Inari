#pragma once

#include "kernel/include/typedefs.h"

#define reciprocal(n) (1 / (n))
#define odd(x) ((x) % 2)
#define even(x) (!odd(x))
#define abs(x) (x < 0 ? -(x) : x)
#define align(val, alg) (((val) + (alg)-1) / (alg) * (alg))
#define min(v0, v1) ((v0) > (v1) ? (v1) : (v0))
#define max(v0, v1) ((v0) > (v1) ? (v0) : (v1))

// https://sourceware.org/git/?p=newlib-cygwin.git;a=blob;f=newlib/libm/common/fdlibm.h;h=7a49e29e0bee3501023779ffbe96f29c609c3f4f;hb=master
typedef union
{
    double value;
    struct
    {
        uint32_t msw;
        uint32_t lsw;
    } parts;
} ieee_double_shape_type;

/* Get two 32 bit ints from a double.  */
#define EXTRACT_WORDS(ix0, ix1, d)   \
    do                               \
    {                                \
        ieee_double_shape_type ew_u; \
        ew_u.value = (d);            \
        (ix0) = ew_u.parts.msw;      \
        (ix1) = ew_u.parts.lsw;      \
    } while (0)

/* Set a double from two 32 bit ints.  */
#define INSERT_WORDS(d, ix0, ix1)    \
    do                               \
    {                                \
        ieee_double_shape_type iw_u; \
        iw_u.parts.msw = (ix0);      \
        iw_u.parts.lsw = (ix1);      \
        (d) = iw_u.value;            \
    } while (0)
// ---

double pow(double base, int exp);
double round(double x);
