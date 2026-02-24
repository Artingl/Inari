#include <kernel/inari.h>
#include <kernel/mm/vmm.h>

#include <arch/x86/tss.h>
#include <arch/x86/gdt.h>
#include <arch/sys.h>

#include <misc/string.h>
#include <misc/types.h>

extern char _arch_stack_bsp;
static uintptr_t kernel_stack;
struct tss_entry_struct tss_entry;
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
    gdt[1].conforming = 0;
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

	// Compute the base and limit of the TSS for use in the GDT entry.
	uint32_t base = (uint32_t) &tss_entry;
	uint32_t limit = sizeof(tss_entry);

	// Add a TSS descriptor to the GDT.
	gdt[5].limit_low = limit;
	gdt[5].base_low = base;
	gdt[5].accessed = 1; // With a system entry (`code_data_segment` = 0), 1 indicates TSS and 0 indicates LDT
	gdt[5].read_write = 0; // For a TSS, indicates busy (1) or not busy (0).
	gdt[5].conforming = 0; // always 0 for TSS
	gdt[5].code = 1; // For a TSS, 1 indicates 32-bit (1) or 16-bit (0).
	gdt[5].code_data_segment = 0; // indicates TSS/LDT (see also `accessed`)
	gdt[5].DPL = 0; // ring 0, see the comments below
	gdt[5].present = 1;
	gdt[5].limit_high = (limit & (0xf << 16)) >> 16; // isolate top nibble
	gdt[5].available = 0; // 0 for a TSS
	gdt[5].long_mode = 0;
	gdt[5].big = 0; // should leave zero according to manuals.
	gdt[5].gran = 0; // limit is in bytes, not pages
	gdt[5].base_high = (base & (0xff << 24)) >> 24; //isolate top byte

	memset(&tss_entry, 0, sizeof tss_entry);

	tss_entry.ss0  = 0x10;
	tss_entry.esp0 = (uint32_t)&_arch_stack_bsp;
    tss_entry.iomap_base = sizeof(struct tss_entry_struct);
    
    __asm__ volatile("ltr %%ax" : : "a" (0x28));
}
