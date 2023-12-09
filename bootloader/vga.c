#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>
#include <bootloader/lower.h>

#include <stdarg.h>

volatile __attribute__((section(".lo_rodata")))
uint8_t lower_vga_x = 0,
        lower_vga_y = 0;

LKERN void lower_vga_init()
{
}

LKERN void lower_vga_print(char *msg)
{
    lower_vga_add(MESSAGES_POOL[MSG_PREFIX]);
    lower_vga_add(msg);
}

LKERN void lower_vga_add(char *addr)
{
    do
    {
        lower_vga_printc(*addr++);
    } while (*addr);
}

LKERN void lower_vga_printc(char c)
{

    if (c == '\n' || lower_vga_x >= 80)
    {
        lower_vga_y++;
        lower_vga_x = 0;

        if (lower_vga_y >= 25)
        {
            lower_vga_y = 79;
            memcpy(
                0xb8000,
                0xb8000 + 80,
                3840);

            memset(0xb8000 + 3840, 0, 80);
        }
        return;
    }

    *((uint16_t *)((0xb8000) + (lower_vga_y * 160 + (lower_vga_x * 2)))) = 0x07 << 8 | c;
    lower_vga_x++;
}

LKERN int __lower_do_printkn(const char *fmt, va_list args, int (*fn)(char, void **), void *ptr);

LKERN int __lower_vga_wh(char c, void **)
{
    lower_vga_printc(c);
}

LKERN void __lower_vga_printf_wrapper(char *fmt, ...)
{
    lower_vga_add(MESSAGES_POOL[MSG_PREFIX]);
    
    va_list args;
    va_start(args, fmt);
    __lower_do_printkn(fmt, args, &__lower_vga_wh, NULL);
    lower_vga_printc('\n');
    va_end(args);
}
