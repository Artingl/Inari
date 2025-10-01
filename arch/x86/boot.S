bits 32
section ._lo_kern_multiboot
    dd 0x1BADB002
    dd 0x00
    dd - (0x1BADB002 + 0x00)

section ._lo_kern_data
global _arch_paging_pool
_arch_paging_pool: resb 0x100000


section ._lo_kern_text
global _arch_entry_point

extern _x86_gdt_descriptor
extern _x86_gdt_code_seg
extern _x86_gdt_data_seg
extern _x86_entrypoint

_arch_entry_point:
    cli
    
    ;; Setup gdt
    lgdt [_x86_gdt_descriptor]
    jmp _x86_gdt_code_seg:.setcs
    .setcs:

    ;; Setup stack
    mov esp, _arch_lo_stack

    ;; push multiboot values onto the stack
    push ebx                                  ;; multiboot2: info structure base
    push eax                                  ;; multiboot2: magic value

    ;; Setup the segment registers with our flat data selector
    mov ax, _x86_gdt_data_seg
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    call _x86_entrypoint
    hlt


section ._lo_kern_stack
global _arch_lo_stack
resb 0xfff
_arch_lo_stack:

section ._hi_kern_stack
global _arch_stack_bsp
resb 0x10000
_arch_stack_bsp:
