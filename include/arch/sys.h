#ifndef _INARI_SYS_H
#define _INARI_SYS_H

#define halt() _arch_hlt()
#define disable_int() _disable_int()
#define enable_int() _enable_int()

extern void _disable_int();
extern void _enable_int();
extern void _arch_hlt();

#endif