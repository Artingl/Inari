#include <sys.h>
#include <lib.h>
#include <string.h>

#include <types.h>

int atoi(const char *str) {
    int res = 0, sign = 1;
    while (isspace((unsigned char)*str)) str++;
    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }
    while (isdigit((unsigned char)*str)) {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

long atol(const char *str) {
    long res = 0;
    int sign = 1;

    while (*str == ' ' || (*str >= 9 && *str <= 13)) str++; // Skip whitespace

    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }

    while (*str >= '0' && *str <= '9') {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res * sign;
}

double atof(const char *str) {
    double res = 0.0, factor = 1.0;
    int sign = 1, decimal_found = 0;

    while (*str == ' ' || (*str >= 9 && *str <= 13)) str++;

    if (*str == '-' || *str == '+') {
        if (*str == '-') sign = -1;
        str++;
    }

    for (; *str; str++) {
        if (*str == '.') {
            decimal_found = 1;
            continue;
        }
        if (*str < '0' || *str > '9') break;
        
        if (!decimal_found) {
            res = res * 10.0 + (*str - '0');
        } else {
            factor /= 10.0;
            res += (*str - '0') * factor;
        }
    }
    return res * sign;
}


long strtol(const char *str, char **endptr, int base) {
    const char *s = str;
    long res = 0;
    int sign = 1;

    while (isspace((unsigned char)*s)) s++; // Skip whitespace

    if (*s == '-' || *s == '+') {
        if (*s == '-') sign = -1;
        s++;
    }

    // Auto-detect base if base is 0
    if (base == 0) {
        if (*s == '0') {
            if (s[1] == 'x' || s[1] == 'X') { base = 16; s += 2; }
            else { base = 8; s++; }
        } else base = 10;
    } else if (base == 16 && *s == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }

    while (*s) {
        int val;
        if (isdigit((unsigned char)*s)) val = *s - '0';
        else if (isalpha((unsigned char)*s)) val = toupper((unsigned char)*s) - 'A' + 10;
        else break;

        if (val >= base) break;
        res = res * base + val;
        s++;
    }

    if (endptr) *endptr = (char *)s;
    return res * sign;
}
