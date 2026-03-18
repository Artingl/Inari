#ifndef _LIBC_MATH_H
#define _LIBC_MATH_H

#define CONST_PI  3.14159265359
#define CONST_2PI 6.28318530718

int abs(int n);
long labs(long n);
long long llabs(long long n);
double absd(double n);
float absf(float n);

double sin(double x);
double cos(double x);

float fmodf(float x, float y);
double fmod(double x, double y);
long double fmodl(long double x, long double y);

#endif
