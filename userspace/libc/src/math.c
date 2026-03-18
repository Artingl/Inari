//  Source: https://austinhenley.com/blog/cosine.html

#include <lib.h>
#include <math.h>

#pragma GCC push_options
#pragma GCC optimize("O3")
#include "costable.h"
#include "sintable.h"

#define lerp(w, v1, v2) ((1.0 - (w)) * (v1) + (w) * (v2))

int abs(int n) { return n < 0 ? -n : n; }

double absd(double n) { return n < 0 ? -n : n; }

float absf(float n) { return n < 0 ? -n : n; }

long labs(long n) { return n < 0 ? -n : n; }

long long llabs(long long n) { return n < 0 ? -n : n; }

double cos(double x) {
    x = absd(x);
    x = fmod(x, CONST_2PI);
    double i = x * 100.0;
    int index = (int)i;
    return lerp(i - index, costable[index], costable[index + 1]);
}

double sin(double x) {
    x = absd(x);
    x = fmod(x, CONST_2PI);
    double i = x * 100.0;
    int index = (int)i;
    return lerp(i - index, sintable[index], sintable[index + 1]);
}

float fmodf(float x, float y) {
    if (y == 0.0f)
        return 0.0f;
    float quotient = x / y;
    long long n = (long long)quotient;
    return x - (n * y);
}

double fmod(double x, double y) {
    if (y == 0.0)
        return 0.0;
    double quotient = x / y;
    long long n = (long long)quotient;
    return x - (n * y);
}

long double fmodl(long double x, long double y) {
    if (y == 0.0L)
        return 0.0L;
    long double quotient = x / y;
    long long n = (long long)quotient;
    return x - (n * y);
}

#pragma GCC pop_options
