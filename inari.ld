OUTPUT_FORMAT(elf32-i386)
ENTRY(_lower_kernel_asm_entry)
SECTIONS
{
    _bootloader_start = .;
    . = 0x100000;

    .bootloader.data : { *(.bootloader.data) }
    .bootloader.text : { *(.bootloader.text) }
    .bootloader.bss : { *(.bootloader.bss) }
    .bootloader.rodata : { *(.bootloader.rodata) }

    _bootloader_end = .;

    . += 0xC0000000;
    _kernel_start = .;
    _kernel_phys_start = . - 0xC0000000;
    
    .text ALIGN (4K) : AT (ADDR (.text) - 0xC0000000) { *(.text) }
    .rodata ALIGN (4K) : AT (ADDR (.rodata) - 0xC0000000) { *(.rodata) }
    .data ALIGN (4K) : AT (ADDR (.data) - 0xC0000000) { *(.data) }
    .bss ALIGN (4K) : AT (ADDR (.bss) - 0xC0000000) { *(.bss) }
    
    _kernel_end = .;
    _kernel_phys_end = . - 0xC0000000;
}