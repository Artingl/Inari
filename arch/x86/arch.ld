ENTRY(_arch_entry_point)
SECTIONS
{
    . = 0x7e00;
    kern_phys_start = .;

    ._lo_kern_multiboot ALIGN (4K) : { *(._lo_kern_multiboot) }
    ._lo_kern_gdt ALIGN (4K) : { *(._lo_kern_gdt) }

    . = 0x100000;
    ._lo_kern_data : { *(._lo_kern_data) }
    ._lo_kern_text : { *(._lo_kern_text) }
    ._lo_kern_stack : { *(._lo_kern_stack) }

    . = ALIGN(0x1000);
    _arch_pagetable_area_start = .;
    . += 0x400000;           /* reserve 4MB */
    _arch_pagetable_area_end = .;


    kern_phys_end = .;
    . += 0xC0000000;
    kern_virt_start = .;

    INCLUDE ../../kernel.ld

    ._hi_kern_stack ALIGN (4K) : AT (ADDR (._hi_kern_stack) - 0xC0000000) { *(._hi_kern_stack) }

    kern_virt_end = .;
}