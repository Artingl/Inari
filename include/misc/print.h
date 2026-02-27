#ifndef _INARI_MISC_PRINT_H
#define _INARI_MISC_PRINT_H

#include <kernel/inari.h>
#include <misc/string.h>
#include <stdarg.h>

#define PR_LJ 0x01
#define PR_CA 0x02
#define PR_SG 0x04
#define PR_32 0x08
#define PR_16 0x10
#define PR_WS 0x20
#define PR_LZ 0x40
#define PR_64 0x60
#define PR_BUFLEN 32

static inline int emit_char(void (*fn)(char, void *), void *ud, char c) {
    fn(c, ud);
    return 1;
}

static inline int emit_string(void (*fn)(char, void *), void *ud, const char *s) {
    int n = 0;
    while (*s)
        n += emit_char(fn, ud, *s++);
    return n;
}

static inline int printnum(void (*fn)(char, void *), void *ud, unsigned long num, unsigned radix, unsigned flags) {
    char buf[PR_BUFLEN];
    char *p = buf + sizeof(buf);

    *--p = '\0';
    do {
        unsigned d = num % radix;
        num /= radix;
        *--p = (d < 10) ? ('0' + d) : ((flags & PR_CA) ? 'A' + d - 10 : 'a' + d - 10);
    } while (num);

    return emit_string(fn, ud, p);
}

static inline int do_kprintfn(const char *fmt, va_list args, void (*fn)(char, void *), void *ud) {
    int count = 0;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            count += emit_char(fn, ud, *fmt);
            continue;
        }

        // ---- Parse flags ----
        unsigned flags = 0, width = 0;
        for (;;) {
            switch (*++fmt) {
            case '-':
                flags |= PR_LJ;
                continue;
            case '0':
                flags |= PR_LZ;
                continue;
            default:
                goto width_parse;
            }
        }

    width_parse:
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // ---- Modifiers ----
        if (*fmt == 'l') {
            flags |= PR_32;
            fmt++;
        } else if (*fmt == 'h') {
            flags |= PR_16;
            fmt++;
        }

        if (*fmt == 'l' && flags & PR_32) {
            flags |= PR_64;
            fmt++;
        }

        // ---- Conversion ----
        unsigned long num;
        switch (*fmt) {
        case 'c':
            count += emit_char(fn, ud, (char)va_arg(args, int));
            break;
        case 's':
            count += emit_string(fn, ud, va_arg(args, const char *));
            break;
        case 'd':
        case 'i':
            flags |= PR_SG; // fallthrough
        case 'u':
            num = (flags & PR_64)   ? va_arg(args, unsigned long long)
                  : (flags & PR_32) ? va_arg(args, unsigned long)
                  : (flags & PR_16) ? (unsigned short)va_arg(args, unsigned int)
                                    : va_arg(args, unsigned int);
            if (flags & PR_SG) {
                long sn = (long)num;
                if (sn < 0) {
                    count += emit_char(fn, ud, '-');
                    num = -sn;
                }
            }
            count += printnum(fn, ud, num, 10, flags);
            break;
        case 'x':
        case 'X':
            if (*fmt == 'X')
                flags |= PR_CA;
            num = (flags & PR_64)   ? va_arg(args, unsigned long long)
                  : (flags & PR_32) ? va_arg(args, unsigned long)
                  : (flags & PR_16) ? (unsigned short)va_arg(args, unsigned int)
                                    : va_arg(args, unsigned int);
            count += printnum(fn, ud, num, 16, flags);
            break;
        case 'o':
            num = va_arg(args, unsigned int);
            count += printnum(fn, ud, num, 8, flags);
            break;
        case 'f': {
            double fnum = va_arg(args, double);

            if (fnum < 0.0) {
                count += emit_char(fn, ud, '-');
                fnum = -fnum;
            }

            double rounding = 0.5;
            double precision = width == 0 ? 6 : (double)width;
            for (unsigned i = 0; i < precision; i++) {
                rounding /= 10.0;
            }
            fnum += rounding;

            unsigned long long int_part = (unsigned long long)fnum;
            double frac_part = fnum - (double)int_part;
            count += printnum(fn, ud, (unsigned long)int_part, 10, 0);

            count += emit_char(fn, ud, '.');
            for (unsigned i = 0; i < precision; i++) {
                frac_part *= 10.0;
                unsigned int digit = (unsigned int)frac_part;
                count += emit_char(fn, ud, '0' + digit);
                frac_part -= digit;
            }
            break;
        }
        case '%':
            count += emit_char(fn, ud, '%');
            break;
        default:
            count += emit_char(fn, ud, *fmt);
            break;
        }
    }
    return count;
}

#endif
