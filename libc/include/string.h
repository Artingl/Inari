#ifndef _INARI_MISC_STRING_H
#define _INARI_MISC_STRING_H

#include <typedefs.h>

/* =========================
   Character helpers
   ========================= */
static inline char k_toupper(char c) {
    return (c >= 'a' && c <= 'z') ? c - 0x20 : c;
}

/* =========================
   Memory functions
   ========================= */
static inline void *memset(void *buf, int ch, size_t count) {
    unsigned char *c = buf;
    while (count--) *c++ = (unsigned char)ch;
    return buf;
}

static inline void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) return *p1 - *p2;
        p1++; p2++;
    }
    return 0;
}

/* =========================
   String functions
   ========================= */
static inline size_t strlen(const char *s) {
    size_t i = 0;
    while (s[i]) i++;
    return i;
}

static inline char *strcpy(char *dest, const char *src) {
    char *ret = dest;
    while ((*dest++ = *src++));
    return ret;
}

static inline char *strncpy(char *dest, const char *src, size_t n) {
    char *ret = dest;
    while (n && (*dest++ = *src++)) n--;
    while (n--) *dest++ = '\0';
    return ret;
}

static inline char *strcat(char *dest, const char *src) {
    char *d = dest + strlen(dest);
    while ((*d++ = *src++));
    return dest;
}

static inline int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static inline int strncmp(const char *s1, const char *s2, size_t n) {
    while (n-- && *s1 && (*s1 == *s2)) { s1++; s2++; }
    return (n == (size_t)-1) ? 0 : *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static inline char *strchr(const char *s, int c) {
    while (*s && *s != (char)c) s++;
    return (*s == (char)c) ? (char*)s : NULL;
}

static inline char *strpbrk(const char *s, const char *accept) {
    for (; *s; s++) {
        for (const char *a = accept; *a; a++)
            if (*s == *a) return (char*)s;
    }
    return NULL;
}

static inline size_t strspn(const char *s1, const char *s2) {
    size_t count = 0;
    for (; *s1 && strchr(s2, *s1); s1++, count++);
    return count;
}

static inline size_t strcspn(const char *s1, const char *s2) {
    size_t count = 0;
    for (; *s1 && !strchr(s2, *s1); s1++, count++);
    return count;
}

static inline char *strstr(const char *s1, const char *s2) {
    if (!*s2) return (char*)s1;
    size_t len2 = strlen(s2);
    for (; (s1 = strchr(s1, *s2)); s1++) {
        if (strncmp(s1, s2, len2) == 0) return (char*)s1;
    }
    return NULL;
}

/* =========================
   Tokenizers
   ========================= */
static inline char *strtok(char *str, const char *delim) {
    static char *p;
    if (str) p = str;
    if (!p) return NULL;

    str = p + strspn(p, delim);
    if (!*str) { p = NULL; return NULL; }

    p = str + strcspn(str, delim);
    if (*p) *p++ = '\0'; else p = NULL;
    return str;
}

static inline char *strtok_r(char *str, const char *delim, char **saveptr) {
    if (!str) str = *saveptr;
    str += strspn(str, delim);
    if (!*str) { *saveptr = str; return NULL; }
    char *token = str;
    str = strpbrk(token, delim);
    if (str) { *str++ = '\0'; *saveptr = str; }
    else *saveptr = token + strlen(token);
    return token;
}

/* =========================
   Misc utils
   ========================= */
static inline void trim(char *c, size_t sz) {
    if (!c || sz == 0) return;
    size_t i = sz - 1;
    while (i > 0 && c[i] == ' ') i--;
    if (c[i] == ' ') c[i] = '\0';
    else c[i + 1] = '\0';
}

static inline char *lfind(const char *str, char accept) {
    while (*str && *str != accept) str++;
    return (*str == accept) ? (char*)str : NULL;
}

/* =========================
   Float to string
   ========================= */
#define K_PRECISION 6
static inline void double2buf(char *buf, double f) {
    int i = 0;
    if (f < 0.0) { buf[i++] = '-'; f = -f; }

    long whole = (long)f;
    double frac = f - whole;

    /* whole part */
    char tmp[32]; int j = 0;
    if (whole == 0) tmp[j++] = '0';
    while (whole > 0) {
        tmp[j++] = '0' + (whole % 10);
        whole /= 10;
    }
    while (j--) buf[i++] = tmp[j];

    buf[i++] = '.';

    /* fractional */
    for (int k = 0; k < K_PRECISION; k++) {
        frac *= 10.0;
        int digit = (int)frac;
        buf[i++] = '0' + digit;
        frac -= digit;
    }
    buf[i] = '\0';
}


#endif
