bits 32
section .bootloader.data
    dd 0x1BADB002   ; Magic number
    dd 0x07         ; Flags
    dd - (0x1BADB002 + 0x07) ; Checksum
    dd 0 	    ; header_addr
    dd 0 	    ; load_addr
    dd 0 	    ; load_end_addr
    dd 0 	    ; bss_end_addr
    dd 0

    dd 1	    ; type
    dq 640	    ; width
    dd 480	    ; height
    dd 32	    ; depth


section .bootloader.text

%include "bootloader/asm/cpu/gdt.asm"
%include "bootloader/asm/video/vga.asm"

global _lower_kernel_asm_entry
global _lower_update_stack_and_jump

extern _pass_higher_kernel
extern _bootloader_end
extern _bootloader_C
extern jump_to_kernel

_lower_kernel_asm_entry:
    lgdt [gdt_descriptor]
    jmp CODE_SEG:.setcs                       ;; Set CS to our 32-bit code selector
    .setcs:
    mov ax, DATA_SEG                          ;; Setup the segment registers with our flat data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, stack_bottom                     ;; set stack pointer
    cli                                       ;; Disable interrupts

    ;; multiboot struct must be inside the ebx register
    mov dword [_lower_multiboot_info_struct], ebx
    push ebx
    
    call init_vga
    call _bootloader_C
    hlt

_lower_update_stack_and_jump:
    mov esp, _higher_stack_bottom
    call jump_to_kernel


section .bootloader.bss
global _lower_multiboot_info_struct
    _lower_multiboot_info_struct: dq 0


global _lower_early_heap_top
    _lower_early_heap_top: dq 0
global _lower_early_heap
    _lower_early_heap: dq 0
global _lower_early_heap_end
    _lower_early_heap_end: dq 0

global _LOWER_MESSAGE_DEBUG
    _LOWER_MESSAGE_DEBUG: db "Debug!", 10, 0
global _LOWER_MESSAGE_FILLING
    _LOWER_MESSAGE_FILLING: db "Filling core directory...", 0
global _LOWER_MESSAGE_DONE
    _LOWER_MESSAGE_DONE: db " done!", 10, 0
global _LOWER_MESSAGE_IDENTIFY
    _LOWER_MESSAGE_IDENTIFY: db "Identifying all necessary addresses to core directory...", 0
global _LOWER_MESSAGE_NO_MMAPS
    _LOWER_MESSAGE_NO_MMAPS: db "No memory mappings were found in the multiboot structure!", 10, 0
global _LOWER_MESSAGE_DIR_DEBUG
    _LOWER_MESSAGE_DIR_DEBUG: db "Core directory info: base = %p, tablePhys = %p", 10, 0
global _LOWER_MESSAGE_HALT
    _LOWER_MESSAGE_HALT: db "System is going to be halted NOW.", 10, 0
global _LOWER_MESSAGE_PASS_CONTROL
    _LOWER_MESSAGE_PASS_CONTROL: db "Paging enabled! Passing control to the higher kernel...", 10, 0

    resb 0xffff
    stack_bottom:


;; higher bootloader part
section .text

global __cpu_idt_load
extern idt_descriptor
__cpu_idt_load:
	lidt [idt_descriptor]
	ret

%include "bootloader/asm/cpu/idt.asm"

global _higher_stack_top
global _higher_stack_bottom

section .bss
    _higher_stack_top:
	resb 0x100000 ;; 1MB of stack
    _higher_stack_bottom:

