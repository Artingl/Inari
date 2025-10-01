#include <misc/math.h>

double pow(double base, int exp)
{
   int res = 1;

   while (exp--)
      res *= base;

   return res;
}