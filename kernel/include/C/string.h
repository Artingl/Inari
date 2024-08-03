#pragma once

#include "kernel/include/C/typedefs.h"

char *strdup(const char *str1);
char k_toupper(char c);
void trim(char *c, int sz);

void *memcpy(void *dest, const void *source, size_t count); /* Copy Memory                            */
void *memset(void *buf, int ch, size_t count);              /* Set memory to a certian value          */
int memcmp(const void *s1, const void *s2, size_t n);       /* Compare memory.                        */

void double2buf(char *buf, double f);

size_t strcspn(const char *s1, const char *s2); 			/* Find length of s1 without charecters in s2.  */
size_t strspn(const char *s1, const char *s2);  			/* Find length of parts of s1 included in s2.   */
char *strtok(char *str, const char *delim);     			/* Cut a string at a certian deliminator.		*/
char *strchr(const char *s, int c);             			/* Find first occurance of c in *s.			    */ 
char *strstr(const char *s1, const char *s2);   			/* Find s2 string in s1.                        */
size_t strlen(const char *s);                   			/* Find string length.                          */
int strcmp(const char *s1, const char *s2);     			/* Compare two strings                          */
char *strcpy(char *dst, char *src);             			/* Copy one string into another.                */
char *strcat(char *dst, char *src);             			/* Add one string to the end of another string. */
int strncmp(const char *s1, const char *s2, size_t n); 		/* Compare n bytes in a string.	        */
char* strncpy(char* destination, const char* source, size_t num);
char * strtok_r(char * str, const char * delim, char ** saveptr);
size_t lfind(const char * str, const char accept);
char * strpbrk(const char * str, const char * accept);
void *memcpy(void *dest, const void *src, size_t n);