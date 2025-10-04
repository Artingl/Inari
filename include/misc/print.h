
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
#define PR_BUFLEN 32

static inline int emit_char(void (*fn)(char), char c) {
    fn(c);
    return 1;
}

static inline int emit_string(void (*fn)(char), const char *s) {
    int n = 0;
    while (*s) n += emit_char(fn, *s++);
    return n;
}

static inline int printnum(void (*fn)(char),
                    unsigned long num, unsigned radix, unsigned flags) {
    char buf[PR_BUFLEN];
    char *p = buf + sizeof(buf);

    *--p = '\0';
    do {
        unsigned d = num % radix;
        num /= radix;
        *--p = (d < 10) ? ('0' + d)
                        : ((flags & PR_CA) ? 'A' + d - 10 : 'a' + d - 10);
    } while (num);

    return emit_string(fn, p);
}

static inline int do_printkn(const char *fmt, va_list args,
                      void (*fn)(char)) {
    int count = 0;

    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            count += emit_char(fn, *fmt);
            continue;
        }

        // ---- Parse flags ----
        unsigned flags = 0, width = 0;
        for (;;) {
            switch (*++fmt) {
            case '-': flags |= PR_LJ; continue;
            case '0': flags |= PR_LZ; continue;
            default: goto width_parse;
            }
        }

    width_parse:
        while (*fmt >= '0' && *fmt <= '9') {
            width = width * 10 + (*fmt - '0');
            fmt++;
        }

        // ---- Modifiers ----
        if (*fmt == 'l') { flags |= PR_32; fmt++; }
        else if (*fmt == 'h') { flags |= PR_16; fmt++; }

        // ---- Conversion ----
        unsigned long num;
        switch (*fmt) {
        case 'c':
            count += emit_char(fn, (char)va_arg(args, int));
            break;
        case 's':
            count += emit_string(fn, va_arg(args, const char *));
            break;
        case 'd': case 'i': flags |= PR_SG; fallthrough;
        case 'u':
            num = (flags & PR_32) ? va_arg(args, unsigned long) :
                  (flags & PR_16) ? (unsigned short)va_arg(args, unsigned int) :
                                    va_arg(args, unsigned int);
            if (flags & PR_SG) {
                long sn = (long)num;
                if (sn < 0) { count += emit_char(fn, '-'); num = -sn; }
            }
            count += printnum(fn, num, 10, flags);
            break;
        case 'x': case 'X':
            if (*fmt == 'X') flags |= PR_CA;
            num = (flags & PR_32) ? va_arg(args, unsigned long) :
                  (flags & PR_16) ? (unsigned short)va_arg(args, unsigned int) :
                                    va_arg(args, unsigned int);
            count += printnum(fn, num, 16, flags);
            break;
        case 'o':
            num = va_arg(args, unsigned int);
            count += printnum(fn, num, 8, flags);
            break;
        case '%':
            count += emit_char(fn, '%');
            break;
        default:
            // unknown specifier, print literally
            count += emit_char(fn, *fmt);
            break;
        }
    }
    return count;
}