#pragma once

#define reciprocal(n) (1 / (n))
#define odd(x) ((x) % 2)
#define even(x) (!odd(x))
#define abs(x) (x<0?-(x):x)
#define align(val, alg) (((val) + (alg) - 1) / (alg) * (alg))
#define min(v0, v1) ((v0) > (v1) ? (v1) : (v0))
#define max(v0, v1) ((v0) > (v1) ? (v0) : (v1))

double pow(double base, int exp);
