#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_VGA_TEXT

#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/errno.h>

#include <misc/string.h>
#include <arch/x86/arch.h>

#define VGA_BASE   0xb8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_SIZE   (VGA_WIDTH*VGA_HEIGHT*2)

#define DEF_COLOR 0x07

static int pc_vga_text_is_initialized = 0;
static int offset = 0, line = 0;

static void vga_putc(const char *s, uint32_t count)
{
    if (!pc_vga_text_is_initialized)
        return;

    uint8_t *vga_base = (uint8_t*)VGA_BASE;

    while (count--)
    {
        char c = *s++;

        switch (c)
        {
            case '\n': {
                offset = 0;
                line++;
                break;
            }
            case '\t': {
                line += 4;
                break;
            }
            default: {
                *(vga_base + (line * VGA_WIDTH + offset) * 2 + 0) = c;
                *(vga_base + (line * VGA_WIDTH + offset) * 2 + 1) = DEF_COLOR;
                offset++;
            }
        }

        if (offset > VGA_WIDTH)
        {
            offset = 0;
            line++;
        }

        if (line >= VGA_HEIGHT)
        {
            memcpy((void*)VGA_BASE, (void*)VGA_BASE + VGA_WIDTH * 2, VGA_SIZE - VGA_WIDTH * 2);
            memset((void*)VGA_BASE + VGA_SIZE - VGA_WIDTH * 2, 0, VGA_WIDTH * 2);
            offset = 0;
            line = VGA_HEIGHT - 1;
        }
    }
}

static struct console_dev console_dev = {
    .name = "pc_vga_text",
    .write = vga_putc,
    .read = NULL,
    .flags = CONSOLE_EARLY | CONSOLE_PRINTK
};

int pc_vga_text_init()
{
    if (pc_vga_text_is_initialized)
        return 0;

    // Clear the screen
    uint32_t *base = (uint32_t*)VGA_BASE;
    while ((uintptr_t)base < VGA_BASE+VGA_SIZE)
        *base++ = 0x00;
    
    pc_vga_text_is_initialized = 1;
    return 0;
}

int pc_vga_text_early_probe()
{
    if (pc_vga_text_init() != 0)
        return -ENODEV;
    return console_register(&console_dev);
}

void pc_vga_text_early_cleanup()
{
}

earlycon_device(
    "pc_vga_text",
    pc_vga_text_early_probe,
    pc_vga_text_early_cleanup
);

#endif
#endif
