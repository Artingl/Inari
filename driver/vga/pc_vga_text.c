#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_VGA_TEXT

#include <kernel/inari.h>
#include <kernel/console/earlycon.h>
#include <kernel/console/console.h>
#include <kernel/errno.h>
#include <kernel/module.h>

#include <misc/string.h>
#include <arch/x86/arch.h>

#define VGA_BASE   0xb8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_SIZE   (VGA_WIDTH*VGA_HEIGHT*2)

#define DEF_COLOR 0x07

static int pc_vga_text_is_initialized = 0;
static int offset = 0, line = 0;

static void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	x86_outb(0x3D4, 0x0A);
	x86_outb(0x3D5, (x86_inb(0x3D5) & 0xC0) | cursor_start);

	x86_outb(0x3D4, 0x0B);
	x86_outb(0x3D5, (x86_inb(0x3D5) & 0xE0) | cursor_end);
}

static void vga_update_cursor(int x, int y)
{
	uint16_t pos = y * VGA_WIDTH + x;

	x86_outb(0x3D4, 0x0F);
	x86_outb(0x3D5, (uint8_t) (pos & 0xFF));
	x86_outb(0x3D4, 0x0E);
	x86_outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

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
                offset += 4;
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
    vga_update_cursor(offset, line);
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

    /* Clear the screen */
    uint32_t *base = (uint32_t*)VGA_BASE;
    while ((uintptr_t)base < VGA_BASE+VGA_SIZE)
        *base++ = 0x00;
    
    // vga_enable_cursor(0, 1);
    pc_vga_text_is_initialized = 1;
    return 0;
}

int pc_vga_text_probe()
{
    if (pc_vga_text_init() != 0)
        return -ENODEV;
    return console_register(&console_dev);
}

void pc_vga_text_cleanup()
{
}

earlycon_device(
    "pc_vga_text",
    pc_vga_text_probe,
    pc_vga_text_cleanup
);

module_t pc_vga_text_module = {
    .probe = pc_vga_text_probe,
    .cleanup = pc_vga_text_cleanup
};


/* This would allow to register this device as normal console after early stage.
 * If it was already initialized, nothing should happen. */
module_register(
    "pc_vga_text",
    pc_vga_text_module
);

#endif
#endif
