section .text
extern x86_irq_handler
extern x86_exception_handler

_arch_excp_base:
    pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
    call x86_exception_handler
	pop gs
	pop fs
	pop es
	pop ds
	popad
    add esp, 8
	sti
	iretd

_arch_irq_base:
    pushad
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
    call x86_irq_handler
	pop gs
	pop fs
	pop es
	pop ds
	popad
    add esp, 8
	sti
	iretd

%macro SWI_HANDLER 1
global _arch_irq%1
_arch_irq%1:
    cli
    push dword 0
    push dword %1
    jmp _arch_irq_base
%endmacro

%macro IRQ_HANDLER 1
global _arch_irq%1
_arch_irq%1:
    cli
    push dword 0
    push dword %1 + 32
    jmp _arch_irq_base
%endmacro

%macro EXCP_HANDLER0 1
global _arch_excp%1
_arch_excp%1:
    cli
    push dword 0
    push dword %1
    jmp _arch_excp_base
%endmacro

%macro EXCP_HANDLER1 1
global _arch_excp%1
_arch_excp%1:
    cli
    push dword %1
    jmp _arch_excp_base
%endmacro

EXCP_HANDLER0 0
EXCP_HANDLER0 1
EXCP_HANDLER0 2
EXCP_HANDLER0 3
EXCP_HANDLER0 4
EXCP_HANDLER0 5
EXCP_HANDLER0 6
EXCP_HANDLER0 7
EXCP_HANDLER1 8
EXCP_HANDLER0 9
EXCP_HANDLER1 10
EXCP_HANDLER1 11
EXCP_HANDLER1 12
EXCP_HANDLER1 13
EXCP_HANDLER1 14
EXCP_HANDLER0 16
EXCP_HANDLER1 17
EXCP_HANDLER0 18
EXCP_HANDLER0 19
EXCP_HANDLER0 20
EXCP_HANDLER1 21
EXCP_HANDLER0 28
EXCP_HANDLER1 29
EXCP_HANDLER1 30

IRQ_HANDLER 0
IRQ_HANDLER 1
IRQ_HANDLER 2
IRQ_HANDLER 3
IRQ_HANDLER 4
IRQ_HANDLER 5
IRQ_HANDLER 6
IRQ_HANDLER 7
IRQ_HANDLER 8
IRQ_HANDLER 9
IRQ_HANDLER 10
IRQ_HANDLER 11
IRQ_HANDLER 12
IRQ_HANDLER 13
IRQ_HANDLER 14
IRQ_HANDLER 15

SWI_HANDLER 100