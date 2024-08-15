OUTPUT_FORMAT(elf32-i386)
ENTRY(i686_multiboot2_entry)
SECTIONS
{
    __kloreal_start = .;
    . = 0x7e00;
    .__klo_gdt : { *(.__klo_gdt) }
    . = 0x100000;

    .__klo_data : { *(.__klo_data) }
    .__klo_text : { *(.__klo_text) }

    . = 0x600000;
    __kloreal_end = .;

    . += 0xC0000000;
    __kvirtual_start = .;
    __kreal_start = . - 0xC0000000;
    
    .text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000) { *(.text) }
    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000) { *(.rodata) }
    .data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000) { *(.data) }
    .bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000) { *(.bss) }
    
    __kvirtual_end = .;
    __kreal_end = . - 0xC0000000;
}