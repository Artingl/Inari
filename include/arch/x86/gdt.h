#ifndef _INARI_X86_GDT_H
#define _INARI_X86_GDT_H

#include <misc/types.h>

struct gdt_entry_bits {
    uint32_t limit_low              : 16;
    uint32_t base_low               : 24;
    uint32_t accessed               :  1;
    uint32_t read_write             :  1;
    uint32_t conforming                :  1;
    uint32_t code                   :  1;
    uint32_t code_data_segment      :  1;
    uint32_t DPL                    :  2;
    uint32_t present                :  1;
    uint32_t limit_high             :  4;
    uint32_t available              :  1;
    uint32_t long_mode              :  1;
    uint32_t big                    :  1;
    uint32_t gran                   :  1;
    uint32_t base_high              :  8;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));


void x86_gdt_init();

#endif