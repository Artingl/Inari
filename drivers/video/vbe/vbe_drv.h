#pragma once

#include <kernel/include/C/typedefs.h>

#include <drivers/video/video.h>

#define VBE_BIOS_INFO 0x4F00


enum
{
    VBE_ERR_FETCH = 1,
    VBE_ERR_SIGNATURE = 2,
};

struct vbe_block_info
{
    char vbe_signature[4];      // == "VESA"
    uint16_t vbe_version;       // == 0x0300 for VBE 3.0
    uint16_t oem_string_ptr[2]; // isa vbeFarPtr
    uint8_t capabilities[4];
    uint16_t video_mode_ptr[2]; // isa vbeFarPtr
    uint16_t total_memory;      // as # of 64KB blocks
    uint8_t reserved[492];
} __attribute__((packed));

struct vbe_mode_info
{
    uint16_t attributes;  // deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
    uint8_t window_a;     // deprecated
    uint8_t window_b;     // deprecated
    uint16_t granularity; // deprecated; used while calculating bank numbers
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr; // deprecated; used to switch banks from protected mode without returning to real mode
    uint16_t pitch;        // number of bytes per horizontal line
    uint16_t width;        // width in pixels
    uint16_t height;       // height in pixels
    uint8_t w_char;        // unused...
    uint8_t y_char;        // ...
    uint8_t planes;
    uint8_t bpp;   // bits per pixel in this mode
    uint8_t banks; // deprecated; total number of banks in this mode
    uint8_t memory_model;
    uint8_t bank_size; // deprecated; size of a bank, almost always 64 KB but may be 16 KB...
    uint8_t image_pages;
    uint8_t reserved0;

    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;

    uint32_t framebuffer; // physical address of the linear frame buffer; write here to draw to the screen
    uint32_t off_screen_mem_off;
    uint16_t off_screen_mem_size; // size of memory in the framebuffer but not being displayed on the screen
    uint8_t reserved1[206];
} __attribute__((packed));

struct vbe_mode
{
    struct vbe_mode_info info;
    uint32_t mode_id;
};

struct kern_video_vbe
{
    struct vbe_block_info vbe_block;
    struct vbe_mode vbe_mode;

    uint32_t framebuffer_back;
};

enum {
    VBE_SUCCESS = 0,
    VBE_INVALID_SIGNATURE = 1
};

int vbe_init(struct kern_video_vbe *video);
int vbe_width();
int vbe_height();
int vbe_bpp();

void vbe_clear();
void vbe_put_pixel(uint32_t x, uint32_t y, uint8_t r, uint8_t g, uint8_t b);

void *vbe_fb();
size_t vbe_fb_size();
