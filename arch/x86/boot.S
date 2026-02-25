bits 32
section ._lo_kern_multiboot
    dd 0x1BADB002
    dd 0x00
    dd - (0x1BADB002 + 0x00)


section ._lo_kern_text
global _arch_entry_point

extern x86_gdt_descriptor
extern x86_gdt_code_seg
extern x86_gdt_data_seg
extern x86_entrypoint

_arch_entry_point:
    cli
    ;; Setup stack
    mov esp, _arch_lo_stack

    ;; push multiboot values onto the stack
    push ebx                                  ;; multiboot: info structure base
    push eax                                  ;; multiboot: magic value
    
    call x86_entrypoint
    hlt


section ._lo_kern_stack
global _arch_lo_stack
resb 0xfff
_arch_lo_stack:

section ._hi_kern_stack
global _arch_stack_bsp
resb 0x10000
_arch_stack_bsp:
