OUTPUT_FORMAT(elf32-i386)
ENTRY(lo_kernel_asm_entry)
SECTIONS
{
    _lo_start_marker = .;
    . = 0x7e00;
    .lo_gdt : { *(.lo_gdt) }
    . = 0x100000;

    .lo_data : { *(.lo_data) }
    .lo_text : { *(.lo_text) }
    .lo_bss : { *(.lo_bss) }
    .lo_rodata : { *(.lo_rodata) }

    _lo_end_marker = .;

    . += 0xC0000000;
    _hi_start_marker = .;
    _kernel_phys_start = . - 0xC0000000;
    
    .text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000) { *(.text) }
    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000) { *(.rodata) }
    .data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000) { *(.data) }
    .bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000) { *(.bss) }
    
    _hi_end_marker = .;
    _kernel_phys_end = . - 0xC0000000;
}