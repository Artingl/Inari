#pragma once

#include <drivers/impl.h>
#include <drivers/video/video.h>

#include <kernel/kernel.h>

#include <kernel/include/C/typedefs.h>

struct kern_video_vga
{
    void *base;       // Pointer to the VGA buffer (usually it is 0xb8000, if not remapped by the bootloader)
    uint32_t rows;
    uint32_t columns;
};

int vga_init(struct kern_video_vga *vga);

void vga_clear();
int vga_print_at(uint32_t *message, uint8_t clr, size_t offset, size_t length);
int vga_set_at(char c, uint8_t clr, size_t offset);

int vga_rows();
int vga_columns();

void vga_render(uint32_t *buffer, size_t width, size_t height);

void *vga_fb();
size_t vga_fb_size();
