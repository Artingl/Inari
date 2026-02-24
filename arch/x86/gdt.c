#include <kernel/inari.h>

#include <arch/x86/gdt.h>

struct gdt_entry_bits {
    unsigned int limit_low              : 16;
    unsigned int base_low               : 24;
    unsigned int accessed               :  1;
    unsigned int read_write             :  1;
    unsigned int conforming                :  1;
    unsigned int code                   :  1;
    unsigned int code_data_segment      :  1;
    unsigned int DPL                    :  2;
    unsigned int present                :  1;
    unsigned int limit_high             :  4;
    unsigned int available              :  1;
    unsigned int long_mode              :  1;
    unsigned int big                    :  1;
    unsigned int gran                   :  1;
    unsigned int base_high              :  8;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry_bits gdt[6] = {0}; /* One null, code/data ring0, code/data ring3, TSS */
static struct gdt_ptr gp = {
    .limit = (sizeof(struct gdt_entry_bits) * 6) - 1,
    .base  = (uint32_t)&gdt[0]
};

void x86_gdt_init()
{
    /* GDT code segment for ring0 */
    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0;
    gdt[1].accessed = 0;
    gdt[1].read_write = 1;
    gdt[1].conforming = 1;
    gdt[1].code = 1;
    gdt[1].code_data_segment = 1;
    gdt[1].DPL = 0;
    gdt[1].present = 1;
    gdt[1].limit_high = 0xF;
    gdt[1].available = 1;
    gdt[1].long_mode = 0;
    gdt[1].big = 1;
    gdt[1].gran = 1;
    gdt[1].base_high = 0;

    /* GDT data segment for ring0 */
    *((struct gdt_entry_bits*)&gdt[2]) = gdt[1];
    gdt[2].code = 0; // not code but data

    /* GDT code segment for ring3 */
    gdt[3].limit_low = 0xFFFF;
    gdt[3].base_low = 0;
    gdt[3].accessed = 0;
    gdt[3].read_write = 1;
    gdt[3].conforming = 0;
    gdt[3].code = 1;
    gdt[3].code_data_segment = 1;
    gdt[3].DPL = 3;
    gdt[3].present = 1;
    gdt[3].limit_high = 0xF;
    gdt[3].available = 1;
    gdt[3].long_mode = 0;
    gdt[3].big = 1;
    gdt[3].gran = 1;
    gdt[3].base_high = 0;

    /* GDT data segment for ring3 */
    *((struct gdt_entry_bits*)&gdt[4]) = gdt[3];
    gdt[4].code = 0; // not code but data

    /* Setup ring0 GDT */
    __asm__ volatile (
        "lgdt %0\n\t"              // Load GDT
        "ljmp $0x08, $1f\n\t"      // Far jump to code segment (0x08)
        "1:\n\t"
        "mov $0x10, %%ax\n\t"      // 0x10 is the data segment selector
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        : 
        : "m"(gp) 
        : "ax", "memory"
    );

}
