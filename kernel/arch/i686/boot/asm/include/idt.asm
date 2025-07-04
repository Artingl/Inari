;; Exceptions
global _excp0
_excp0:
        cli
        push dword 0
        push dword 0
        jmp int_excp_stub
global _excp1
_excp1:
        cli
        push dword 0
        push dword 1
        jmp int_excp_stub
global _excp2
_excp2:
        cli
        push dword 0
        push dword 2
        jmp int_excp_stub
global _excp3
_excp3:
        cli
        push dword 0
        push dword 3
        jmp int_excp_stub
global _excp4
_excp4:
        cli
        push dword 0
        push dword 4
        jmp int_excp_stub
global _excp5
_excp5:
        cli
        push dword 0
        push dword 5
        jmp int_excp_stub
global _excp6
_excp6:
        cli
        push dword 0
        push dword 6
        jmp int_excp_stub
global _excp7
_excp7:
        cli
        push dword 0
        push dword 7
        jmp int_excp_stub
global _excp8
_excp8:
        cli
        push dword 8
        jmp int_excp_stub
global _excp9
_excp9:
        cli
        push dword 0
        push dword 9
        jmp int_excp_stub
global _excp10
_excp10:
        cli
        push dword 10
        jmp int_excp_stub
global _excp11
_excp11:
        cli
        push dword 11
        jmp int_excp_stub
global _excp12
_excp12:
        cli
        push dword 12
        jmp int_excp_stub
global _excp13
_excp13:
        cli
        push dword 13
        jmp int_excp_stub
global _excp14
_excp14:
        cli
        push dword 14
        jmp int_excp_stub
global _excp16
_excp16:
        cli
        push dword 0
        push dword 16
        jmp int_excp_stub
global _excp17
_excp17:
        cli
        push dword 17
        jmp int_excp_stub
global _excp18
_excp18:
        cli
        push dword 0
        push dword 18
        jmp int_excp_stub
global _excp19
_excp19:
        cli
        push dword 0
        push dword 19
        jmp int_excp_stub
global _excp20
_excp20:
        cli
        push dword 0
        push dword 20
        jmp int_excp_stub
global _excp21
_excp21:
        cli
        push dword 21
        jmp int_excp_stub
global _excp28
_excp28:
        cli
        push dword 0
        push dword 28
        jmp int_excp_stub
global _excp29
_excp29:
        cli
        push dword 29
        jmp int_excp_stub
global _excp30
_excp30:
        cli
        push dword 30
        jmp int_excp_stub


;; IRQs
global _irq0
_irq0:
	cli
	push dword 0
	push dword 32
	jmp int_isr_stub
global _irq1
_irq1:
	cli
	push dword 0
	push dword 33
	jmp int_isr_stub
global _irq2
_irq2:
	cli
	push dword 0
	push dword 34
	jmp int_isr_stub
global _irq3
_irq3:
	cli
	push dword 0
	push dword 35
	jmp int_isr_stub
global _irq4
_irq4:
	cli
	push dword 0
	push dword 36
	jmp int_isr_stub
global _irq5
_irq5:
	cli
	push dword 0
	push dword 37
	jmp int_isr_stub
global _irq6
_irq6:
	cli
	push dword 0
	push dword 38
	jmp int_isr_stub
global _irq7
_irq7:
	cli
	push dword 0
	push dword 39
	jmp int_isr_stub
global _irq8
_irq8:
	cli
	push dword 0
	push dword 40
	jmp int_isr_stub
global _irq9
_irq9:
	cli
	push dword 0
	push dword 41
	jmp int_isr_stub
global _irq10
_irq10:
	cli
	push dword 0
	push dword 42
	jmp int_isr_stub
global _irq11
_irq11:
	cli
	push dword 0
	push dword 43
	jmp int_isr_stub
global _irq12
_irq12:
	cli
	push dword 0
	push dword 44
	jmp int_isr_stub
global _irq13
_irq13:
	cli
	push dword 0
	push dword 45
	jmp int_isr_stub
global _irq14
_irq14:
	cli
	push dword 0
	push dword 46
	jmp int_isr_stub
global _irq15
_irq15:
	cli
	push dword 0
	push dword 47
	jmp int_isr_stub
global _syscall
_syscall:
	cli
	push dword 0
	push dword 0x80
	jmp int_isr_stub

extern isr_handler
int_isr_stub:
	pushad
	push esp
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	push esp
	call isr_handler
	pop esp
	pop gs
	pop fs
	pop es
	pop ds
	pop esp
	popa
	add esp, 8
	sti
	iretd

extern isr_handler
int_excp_stub:
	pushad
	push esp
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	push esp
	call isr_handler
	pop esp
	pop gs
	pop fs
	pop es
	pop ds
	pop esp
	popa
	add esp, 8
	sti
	iretd