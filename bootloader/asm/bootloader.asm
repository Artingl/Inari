bits 32
section .__klo_data
    dd 0x1BADB002   ; Magic number
    dd 0x03         ; Flags
    dd - (0x1BADB002 + 0x03) ; Checksum
    dd 0 	    ; header_addr
    dd 0 	    ; load_addr
    dd 0 	    ; load_end_addr
    dd 0 	    ; bss_end_addr
    dd 0

    dd 1	    ; type
    dq 800	    ; width
    dd 600	    ; height
    dd 32	    ; depth



section .__klo_gdt
%include "bootloader/asm/cpu/gdt.asm"

section .__klo_text

global __klo_stack
global __klo_i386_entry

extern i386_entrypoint

__klo_i386_entry:
    lgdt [gdt_descriptor]
    jmp CODE_SEG:.setcs                       ;; Set CS to our 32-bit code selector
    .setcs:
    mov ax, DATA_SEG                          ;; Setup the segment registers with our flat data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, __klo_stack                      ;; set stack pointer
    cli                                       ;; Disable interrupts

    ;; multiboot struct must be inside the ebx register
    ; mov dword [_lower_multiboot_info_struct], ebx
    ; push ebx
    
    call i386_entrypoint
    hlt

resb 0xffff
__klo_stack:

;; higher bootloader part
section .text
%include "bootloader/asm/cpu/idt.asm"

global __kstack_top_bsp
global __kstack_bottom_bsp

section .bss
    __kstack_top_bsp:
	resb 0x2000   ;; 4KB of stack
    __kstack_bottom_bsp:

