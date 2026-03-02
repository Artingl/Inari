#ifdef CONFIG_ARCH_X86
#ifdef CONFIG_DRV_VGA_TEXT

#include <kernel/console/console.h>
#include <kernel/console/earlycon.h>
#include <kernel/errno.h>
#include <kernel/inari.h>
#include <kernel/module.h>

#include <arch/x86/arch.h>
#include <misc/string.h>

#define VGA_BASE   0xb8000
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_SIZE   (VGA_WIDTH * VGA_HEIGHT * 2)

#define DEF_COLOR 0x07

static uint8_t shadow_buffer[VGA_SIZE];

static int pc_vga_text_is_initialized = 0;
static int offset = 0, line = 0;
static uint8_t *vga_shadow_base = (uint8_t *)&shadow_buffer[0];

static void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end) {
    x86_outb(0x3D4, 0x0A);
    x86_outb(0x3D5, (x86_inb(0x3D5) & 0x1F) | cursor_start);

    x86_outb(0x3D4, 0x0B);
    x86_outb(0x3D5, (x86_inb(0x3D5) & 0xE0) | cursor_end);
}

static void vga_update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;

    x86_outb(0x3D4, 0x0F);
    x86_outb(0x3D5, (uint8_t)(pos & 0xFF));
    x86_outb(0x3D4, 0x0E);
    x86_outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

static void vga_flush_buffer() {
    memcpy((void *)VGA_BASE, vga_shadow_base, VGA_SIZE);
    vga_update_cursor(offset, line);
}

static void vga_clear() {
    /* Clear the screen */
    uint8_t *base = (uint8_t *)vga_shadow_base;
    while (base < vga_shadow_base + VGA_SIZE) {
        *base++ = ' ';
        *base++ = DEF_COLOR;
    }

    offset = 0;
    line = 0;
    vga_flush_buffer();
}

static void vga_rewind(uint32_t count, int clear) {
    if (!pc_vga_text_is_initialized)
        return;

    while (count-- && offset > 0) {
        offset--;
        if (clear) {
            *(vga_shadow_base + (line * VGA_WIDTH + offset) * 2 + 0) = ' ';
            *(vga_shadow_base + (line * VGA_WIDTH + offset) * 2 + 1) = DEF_COLOR;
        }

        if (offset <= 0) {
            line--;
            offset = VGA_WIDTH - 1;
        }
        if (line <= 0) {
            line = 0;
        }
    }
    vga_flush_buffer();
}

static void vga_putc(const char *s, uint32_t count) {
    if (!pc_vga_text_is_initialized)
        return;

    while (count--) {
        char c = *s++;

        switch (c) {
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
            *(vga_shadow_base + (line * VGA_WIDTH + offset) * 2 + 0) = c;
            *(vga_shadow_base + (line * VGA_WIDTH + offset) * 2 + 1) = DEF_COLOR;
            offset++;
        }
        }

        if (offset >= VGA_WIDTH) {
            offset = 0;
            line++;
        }

        if (line >= VGA_HEIGHT) {
            memcpy((void *)vga_shadow_base, (void *)vga_shadow_base + VGA_WIDTH * 2, VGA_SIZE - VGA_WIDTH * 2);
            memset((void *)vga_shadow_base + VGA_SIZE - VGA_WIDTH * 2, 0, VGA_WIDTH * 2);
            uint8_t *last_line = (uint8_t *)((void *)vga_shadow_base + VGA_SIZE - VGA_WIDTH * 2);
            for (int i = 0; i < VGA_WIDTH * 2; i += 2) {
                last_line[i] = ' ';
                last_line[i + 1] = DEF_COLOR;
            }
            offset = 0;
            line = VGA_HEIGHT - 1;
            vga_flush_buffer();
        }
    }
}

static struct console_dev console_dev = {.name = "vga_text",
                                         .write = vga_putc,
                                         .rewind = vga_rewind,
                                         .clear = vga_clear,
                                         .flush = vga_flush_buffer,
                                         .read = NULL,
                                         .flags = CONSOLE_EARLY | CONSOLE_PRINT};

static int pc_vga_text_init() {
    if (pc_vga_text_is_initialized)
        return 0;

    vga_enable_cursor(14, 15);
    vga_update_cursor(0, 0);
    vga_clear();

    console_register(&console_dev);
    pc_vga_text_is_initialized = 1;
    return 0;
}

static int pc_vga_text_probe() {
    if (pc_vga_text_init() != 0)
        return -ENODEV;
    if (!pc_vga_text_is_initialized)
        return console_register(&console_dev);
    return 0;
}

static void pc_vga_text_cleanup() {
    console_unregister(&console_dev);
    uint8_t *base = (uint8_t *)VGA_BASE;
    while ((uintptr_t)base < VGA_BASE + VGA_SIZE * 2)
        *base++ = 0;
    vga_flush_buffer();
    pc_vga_text_is_initialized = 0;
}

earlycon_device("vga_text", pc_vga_text_probe, pc_vga_text_cleanup);

module_t pc_vga_text_module = {.probe = pc_vga_text_probe, .cleanup = pc_vga_text_cleanup};

/* This would allow to register this device as normal console after early stage.
 * If it was already initialized, nothing should happen. */
module_register("vga_text", pc_vga_text_module);

#endif
#endif
