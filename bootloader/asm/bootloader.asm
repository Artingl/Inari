bits 32
section .lo_data
    dd 0x1BADB002   ; Magic number
    dd 0x07         ; Flags
    dd - (0x1BADB002 + 0x07) ; Checksum
    dd 0 	    ; header_addr
    dd 0 	    ; load_addr
    dd 0 	    ; load_end_addr
    dd 0 	    ; bss_end_addr
    dd 0

    dd 1	    ; type
    dq 800	    ; width
    dd 600	    ; height
    dd 32	    ; depth



section .lo_gdt
%include "bootloader/asm/cpu/gdt.asm"

section .lo_text
%include "bootloader/asm/video/vga.asm"

global lo_kernel_asm_entry
global lo_update_stack_and_jump
global lo_stack_bottom

extern _lo_end_marker
extern lo_kmain
extern jump_to_kernel

lo_kernel_asm_entry:
    lgdt [gdt_descriptor]
    jmp CODE_SEG:.setcs                       ;; Set CS to our 32-bit code selector
    .setcs:
    mov ax, DATA_SEG                          ;; Setup the segment registers with our flat data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, lo_stack_bottom                     ;; set stack pointer
    cli                                       ;; Disable interrupts

    ;; multiboot struct must be inside the ebx register
    mov dword [_lower_multiboot_info_struct], ebx
    push ebx
    
    call init_vga
    call lo_kmain
    hlt

lo_update_stack_and_jump:
    mov esp, hi_stack_bottom
    call jump_to_kernel

section .lo_bss
global _lower_multiboot_info_struct
    _lower_multiboot_info_struct: dq 0


global lo_early_heap_top
    lo_early_heap_top: dq 0
global lo_early_heap
    lo_early_heap: dq 0
global lo_early_heap_end
    lo_early_heap_end: dq 0

global LO_MESSAGE_DEBUG
    LO_MESSAGE_DEBUG: db "Debug!", 10, 0
global LO_MESSAGE_FILLING
    LO_MESSAGE_FILLING: db "Filling core directory...", 0
global LO_MESSAGE_DONE
    LO_MESSAGE_DONE: db " done!", 10, 0
global LO_MESSAGE_IDENTIFY
    LO_MESSAGE_IDENTIFY: db "Identifying all necessary addresses to core directory...", 0
global LO_MESSAGE_NO_MMAPS
    LO_MESSAGE_NO_MMAPS: db "No memory mappings were found in the multiboot structure!", 10, 0
global LO_MESSAGE_DIR_DEBUG
    LO_MESSAGE_DIR_DEBUG: db "Core directory info: base = %p, tablePhys = %p", 10, 0
global LO_MESSAGE_HALT
    LO_MESSAGE_HALT: db "System is going to be halted NOW.", 10, 0
global LO_MESSAGE_PASS_CONTROL
    LO_MESSAGE_PASS_CONTROL: db "Paging enabled! Passing control to the higher kernel...", 10, 0

    resb 0xffff
    lo_stack_bottom:

;; higher bootloader part
section .text
%include "bootloader/asm/cpu/idt.asm"

global hi_stack_top
global hi_stack_bottom

section .bss
    hi_stack_top:
	resb 0x100000 ;; 1MB of stack
                  ;; TODO: allocate kernel stack on the heap later, so we don't waste memory here
    hi_stack_bottom:

