#include "kernel/include/io.h"
#include "kernel/include/string.h"

#include <stdarg.h>

#define PR_LJ 0x01
#define PR_CA 0x02
#define PR_SG 0x04
#define PR_32 0x08
#define PR_16 0x10
#define PR_WS 0x20
#define PR_LZ 0x40
#define PR_FP 0x80
#define PR_BUFLEN 32

int do_printkn(const char *fmt, va_list args, int (*fn)(char, void **), void *ptr)
{
	unsigned flags, actual_wd, count, given_wd;
	unsigned char *where, buf[PR_BUFLEN];
	unsigned char state, radix, is_ptr;
	long num;
	char fbuf[32];

	state = flags = count = given_wd = 0;

	for (; *fmt; fmt++) /* Begin scanning format specifier list */
	{
		is_ptr = false;
		switch (state)
		{

		case 0:				 /* STATE 0: AWAITING % */
			if (*fmt != '%') /* not %... */
			{
				fn(*fmt, &ptr); /* ...just echo it */
				count++;
				break;
			}
			state++; /* found %, get next char and advance state to check if next char is a flag */
			fmt++;

		case 1:				 /* STATE 1: AWAITING FLAGS (%-0) */
			if (*fmt == '%') /* %% */
			{
				fn(*fmt, &ptr);
				count++;
				state = flags = given_wd = 0;
				break;
			}

			if (*fmt == '-')
			{
				if (flags & PR_LJ) /* %-- is illegal */
					state = flags = given_wd = 0;
				else
					flags |= PR_LJ;
				break;
			}

			state++; /* not a flag char: advance state to check if it's field width */

			if (*fmt == '0') /* check now for '%0...' */
			{
				flags |= PR_LZ;
				fmt++;
			}

		case 2: /* STATE 2: AWAITING (NUMERIC) FIELD WIDTH */
			if (*fmt >= '0' && *fmt <= '9')
			{
				given_wd = 10 * given_wd +
						   (*fmt - '0');
				break;
			}
			state++;

		case 3: /* STATE 3: AWAITING MODIFIER CHARS (FNlh) */
			if (*fmt == 'F')
			{
				flags |= PR_FP;
				break;
			}
			if (*fmt == 'N')
				break;
			if (*fmt == 'l')
			{
				flags |= PR_32;
				break;
			}
			if (*fmt == 'h')
			{
				flags |= PR_16;
				break;
			}
			state++;

		case 4: /* STATE 4: AWAITING CONVERSION CHARS (Xxpndiuocs) */
			where = buf + PR_BUFLEN - 1;
			*where = '\0';
			switch (*fmt)
			{
			case 'f':
				memset(fbuf, '0', 32);
				double2buf(fbuf, va_arg(args, double));

				flags &= ~PR_LZ;
				where = fbuf;
				goto EMIT;
			case 'X':
				flags |= PR_CA;
			/* xxx - far pointers (%Fp, %Fn) not yet supported */
			case 'x':
			case 'n':
				radix = 16;
				goto DO_NUM;
			case 'p':
				radix = 16;
				is_ptr = true;
				goto DO_NUM;
			case 'd':
			case 'i':
				flags |= PR_SG;
			case 'u':
				radix = 10;
				goto DO_NUM;
			case 'o':
				radix = 8;

			DO_NUM:
				if (flags & PR_32) /* load the value to be printed. l=long=32 bits: */
					num = va_arg(args, unsigned long);
				else if (flags & PR_16) /* h=short=16 bits (signed or unsigned) */
				{
					if (flags & PR_SG)
						num = va_arg(args, int);
					else
						num = va_arg(args, unsigned int);
				}

				else /* no h nor l: sizeof(int) bits (signed or unsigned) */
				{
					if (flags & PR_SG)
						num = va_arg(args, int);
					else
						num = va_arg(args, unsigned int);
				}

				if (flags & PR_SG) /* take care of sign */
				{
					if (num < 0)
					{
						flags |= PR_WS;
						num = -num;
					}
				}

				do /* Convert binary to octal/decimal/hex ASCII */
				{
					unsigned long temp;

					temp = (unsigned long)num % radix;
					where--;
					if (temp < 10)
						*where = temp + '0';
					else if (flags & PR_CA)
						*where = temp - 10 + 'A';
					else
						*where = temp - 10 + 'a';
					num = (unsigned long)num / radix;
				} while (num != 0);
				goto EMIT;

			case 'c': /* Disallow pad-left-with-zeroes for %c */
				flags &= ~PR_LZ;
				where--;
				*where = (unsigned char)va_arg(args, unsigned int);
				actual_wd = 1;
				goto EMIT2; // Holy Crap, a Goto!

			case 's': /* Disallow pad-left-with-zeroes for %s */
				flags &= ~PR_LZ;
				where = va_arg(args, unsigned char *);
			EMIT:
				actual_wd = strlen(where);

				if (is_ptr)
				{
					fn('0', &ptr);
					fn('x', &ptr);
				}

				if (flags & PR_WS)
					actual_wd++;

				if ((flags & (PR_WS | PR_LZ)) == (PR_WS | PR_LZ)) /* If we pad left with ZEROES, do the sign now */
				{
					fn('-', &ptr);
					count++;
				}

			EMIT2:
				if ((flags & PR_LJ) == 0) /* Pad on left with spaces or zeroes (for right justify) */
				{
					while (given_wd > actual_wd)
					{
						fn(flags & PR_LZ ? '0' : ' ', &ptr);
						count++;
						given_wd--;
					}
				}

				if ((flags & (PR_WS | PR_LZ)) == PR_WS) /* If we pad left with SPACES, do the sign now */
				{
					fn('-', &ptr);
					count++;
				}

				while (*where != '\0') /* Emit string/char/converted number */
				{
					fn(*where++, &ptr);
					count++;
				}

				if (given_wd < actual_wd) /* Pad on right with spaces (for left justify) */
					given_wd = 0;

				else
					given_wd -= actual_wd;

				for (; given_wd; given_wd--)
				{
					fn(' ', &ptr);
					count++;
				}
				break;

			default:
				break;
			}
		default:
			state = flags = given_wd = 0;
			break;
		}
	}
	return count;
}
