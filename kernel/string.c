#include <misc/string.h>
#include <misc/types.h>

void *memset(void *buf, int ch, size_t count) {
    volatile unsigned char *c = buf;
    while (count--)
        *c++ = (unsigned char)ch;
    return buf;
}

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    volatile unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    volatile const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2)
            return *p1 - *p2;
        p1++;
        p2++;
    }
    return 0;
}
