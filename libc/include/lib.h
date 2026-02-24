#ifndef _LIBC_ALLOC_H
#define _LIBC_ALLOC_H


void *malloc(size_t size);
void free(void *ptr);

int atoi(const char *str);
long atol(const char *str);
double atof(const char *str);
long strtol(const char *str, char **endptr, int base);

#endif


