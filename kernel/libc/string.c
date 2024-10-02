#include <kernel/kernel.h>

#include <kernel/libc/io.h>
#include <kernel/libc/string.h>
#include <kernel/libc/math.h>

char *strdup(const char *str1)
{
	// todo
	return NULL;
	// size_t s = strlen(str1);
	// char *c = kmalloc(s);
	// memcpy(c, str1, s);
	// return c;
}

char k_toupper(char c)
{
	if (c >= 0x61 && c <= 0x7a)
		return c - 0x20;

	return c;
}

void trim(char *c, int sz)
{
	size_t i;
	for (i = sz - 1; i >= 0 && c[i] == ' '; --i)
		;
	if (i < sz - 1)
		c[i + 1] = '\0';
}

void *memset(void *buf, int ch, size_t count)
{
	char *c = (char *)buf;
	for (size_t i = 0; i < count; i++)
		*c++ = ch;
}

int memcmp(const void *s1, const void *s2, size_t n)
{
	const unsigned char *us1 = (const unsigned char *)s1;
	const unsigned char *us2 = (const unsigned char *)s2;
	while (n-- != 0)
	{
		if (*us1 != *us2)
		{
			return (*us1 < *us2) ? -1 : +1;
		}
		us1++;
		us2++;
	}
	return 0;
}

char *strchr(const char *s, int c)
{
	while (*s != (char)c)
	{
		if (!*s++)
		{
			return 0;
		}
	}
	return (char *)s;
}

size_t strspn(const char *s1, const char *s2)
{
	size_t ret = 0;
	while (*s1 && strchr(s2, *s1++))
	{
		ret++;
	}
	return ret;
}

size_t strcspn(const char *s1, const char *s2)
{
	size_t ret = 0;
	while (*s1)
	{
		if (strchr(s2, *s1))
		{
			return ret;
		}
		else
		{
			s1++, ret++;
		}
	}
	return ret;
}

char *strtok(char *str, const char *delim)
{
	static char *p = 0;
	if (str)
	{			 /* Does the string exist? */
		p = str; /* If so, change p to current position. */
	}
	else if (!p)
	{
		return 0; /* Otherwise, exit null. */
	}
	str = p + strspn(p, delim);
	p = str + strcspn(str, delim);
	if (p == str)
	{
		return p = 0;
	}
	p = *p ? *p = 0, p + 1 : 0;
	return str;
}

char *strstr(const char *s1, const char *s2)
{
	size_t s2len;
	if (*s2 == '\0')
	{
		return (char *)s1;
	}
	s2len = strlen(s2);
	for (; (s1 = strchr(s1, *s2)) != NULL; s1++)
	{
		if (strncmp(s1, s2, s2len) == 0)
		{
			return (char *)s1;
		}
	}
	return NULL;
}

size_t strlen(const char *str)
{
	int i = 0;
	while (str[i] != (char)0)
	{
		++i;
	}
	return i;
}

int strcmp(const char *s1, const char *s2)
{
	for (; *s1 == *s2; ++s1, ++s2)
	{
		if (*s1 == 0)
		{
			return 0;
		}
	}
	return *(unsigned char *)s1 < *(unsigned char *)s2 ? -1 : 1;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
	unsigned char uc1, uc2;
	if (n == 0)
		return 0;
	while (n-- > 0 && *s1 == *s2)
	{
		if (n == 0 || *s1 == '\0')
		{
			return 0;
		}
		s1++;
		s2++;
	}
	uc1 = (*(unsigned char *)s1);
	uc2 = (*(unsigned char *)s2);
	return ((uc1 < uc2) ? -1 : (uc1 > uc2));
}

char *strcpy(char *dest, char *src)
{
	do
	{
		*dest++ = *src++;
	} while (*src != 0);
	return dest;
}

char *strcat(char *dst, char *src)
{
	size_t len;

	len = strlen(dst);
	strcpy(dst + len, src);
	return dst;
}

char *strncpy(char *destination, const char *source, size_t num)
{
	// return if no memory is allocated to the destination
	if (destination == NULL)
	{
		return NULL;
	}

	// take a pointer pointing to the beginning of the destination string
	char *ptr = destination;

	// copy first `num` characters of C-string pointed by source
	// into the array pointed by destination
	while (*source && num--)
	{
		*destination = *source;
		destination++;
		source++;
	}

	// null terminate destination string
	*destination = '\0';

	// the destination is returned by standard `strncpy()`
	return ptr;
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
	char *token;
	if (str == NULL)
	{
		str = *saveptr;
	}
	str += strspn(str, delim);
	if (*str == '\0')
	{
		*saveptr = str;
		return NULL;
	}
	token = str;
	str = strpbrk(token, delim);
	if (str == NULL)
	{
		*saveptr = (char *)lfind(token, '\0');
	}
	else
	{
		*str = '\0';
		*saveptr = str + 1;
	}
	return token;
}

size_t lfind(const char *str, const char accept)
{
	size_t i = 0;
	while (str[i] != accept)
	{
		i++;
	}
	return (size_t)(str) + i;
}

char *strpbrk(const char *str, const char *accept)
{
	const char *acc = accept;

	if (!*str)
	{
		return NULL;
	}

	while (*str)
	{
		for (acc = accept; *acc; ++acc)
		{
			if (*str == *acc)
			{
				break;
			}
		}
		if (*acc)
		{
			break;
		}
		++str;
	}

	if (*acc == '\0')
	{
		return NULL;
	}

	return (char *)str;
}

void *memcpy(void *dest, const void *src, size_t n)
{
	char *cdest = dest;
	const char *csrc = src;

	size_t i;
	for (i = 0; i < n; i++)
	{
		cdest[i] = csrc[i];
	}

	return dest;
}

#define precision 6
void double2buf(char *buf, double f)
{
	int a, b, c, k, l = 0, m, i = 0, j;

	// check for negative float
	if (f < 0.0)
	{

		buf[i++] = '-';
		f *= -1;
	}

	a = f;	// extracting whole number
	f -= a; // extracting decimal part
	k = precision;

	// number of digits in whole number
	while (k > -1)
	{
		l = pow(10, k);
		m = a / l;
		if (m > 0)
		{
			break;
		}
		k--;
	}

	// number of digits in whole number are k+1

	/*
	extracting most significant digit i.e. right most digit , and concatenating to string
	obtained as quotient by dividing number by 10^k where k = (number of digit -1)
	*/

	for (l = k + 1; l > 0; l--)
	{
		b = pow(10, l - 1);
		c = a / b;
		buf[i++] = c + 48;
		a %= b;
	}
	if (i == 0)
		buf[i++] = '0';
	buf[i++] = '.';

	/* extracting decimal digits till precision */

	for (l = 0; l < precision; l++)
	{
		f *= 10.0;
		b = f;
		buf[i++] = b + 48;
		f -= b;
	}

	buf[i] = '\0';
}