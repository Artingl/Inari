;; Exceptions
global _excp0
_excp0:
        cli
        push byte 0
        push byte 0
        jmp idt_excp_stub
global _excp1
_excp1:
        cli
        push byte 0
        push byte 1
        jmp idt_excp_stub
global _excp2
_excp2:
        cli
        push byte 0
        push byte 2
        jmp idt_excp_stub
global _excp3
_excp3:
        cli
        push byte 0
        push byte 3
        jmp idt_excp_stub
global _excp4
_excp4:
        cli
        push byte 0
        push byte 4
        jmp idt_excp_stub
global _excp5
_excp5:
        cli
        push byte 0
        push byte 5
        jmp idt_excp_stub
global _excp6
_excp6:
        cli
        push byte 0
        push byte 6
        jmp idt_excp_stub
global _excp7
_excp7:
        cli
        push byte 0
        push byte 7
        jmp idt_excp_stub
global _excp8
_excp8:
        cli
        push byte 8
        jmp idt_excp_stub
global _excp9
_excp9:
        cli
        push byte 0
        push byte 9
        jmp idt_excp_stub
global _excp10
_excp10:
        cli
        push byte 10
        jmp idt_excp_stub
global _excp11
_excp11:
        cli
        push byte 11
        jmp idt_excp_stub
global _excp12
_excp12:
        cli
        push byte 12
        jmp idt_excp_stub
global _excp13
_excp13:
        cli
        push byte 13
        jmp idt_excp_stub
global _excp14
_excp14:
        cli
        push byte 14
        jmp idt_excp_stub
global _excp16
_excp16:
        cli
        push byte 0
        push byte 16
        jmp idt_excp_stub
global _excp17
_excp17:
        cli
        push byte 17
        jmp idt_excp_stub
global _excp18
_excp18:
        cli
        push byte 0
        push byte 18
        jmp idt_excp_stub
global _excp19
_excp19:
        cli
        push byte 0
        push byte 19
        jmp idt_excp_stub
global _excp20
_excp20:
        cli
        push byte 0
        push byte 20
        jmp idt_excp_stub
global _excp21
_excp21:
        cli
        push byte 21
        jmp idt_excp_stub
global _excp28
_excp28:
        cli
        push byte 0
        push byte 28
        jmp idt_excp_stub
global _excp29
_excp29:
        cli
        push byte 29
        jmp idt_excp_stub
global _excp30
_excp30:
        cli
        push byte 30
        jmp idt_excp_stub


;; IRQs
global _irq0
_irq0:
	cli
	push byte 0
	push byte 32
	jmp isr_excp_stub
global _irq1
_irq1:
	cli
	push byte 0
	push byte 33
	jmp isr_excp_stub
global _irq2
_irq2:
	cli
	push byte 0
	push byte 34
	jmp isr_excp_stub
global _irq3
_irq3:
	cli
	push byte 0
	push byte 35
	jmp isr_excp_stub
global _irq4
_irq4:
	cli
	push byte 0
	push byte 36
	jmp isr_excp_stub
global _irq5
_irq5:
	cli
	push byte 0
	push byte 37
	jmp isr_excp_stub
global _irq6
_irq6:
	cli
	push byte 0
	push byte 38
	jmp isr_excp_stub
global _irq7
_irq7:
	cli
	push byte 0
	push byte 39
	jmp isr_excp_stub
global _irq8
_irq8:
	cli
	push byte 0
	push byte 40
	jmp isr_excp_stub
global _irq9
_irq9:
	cli
	push byte 0
	push byte 41
	jmp isr_excp_stub
global _irq10
_irq10:
	cli
	push byte 0
	push byte 42
	jmp isr_excp_stub
global _irq11
_irq11:
	cli
	push byte 0
	push byte 43
	jmp isr_excp_stub
global _irq12
_irq12:
	cli
	push byte 0
	push byte 44
	jmp isr_excp_stub
global _irq13
_irq13:
	cli
	push byte 0
	push byte 45
	jmp isr_excp_stub
global _irq14
_irq14:
	cli
	push byte 0
	push byte 46
	jmp isr_excp_stub
global _irq15
_irq15:
	cli
	push byte 0
	push byte 47
	jmp isr_excp_stub


extern isr_handler
isr_excp_stub:
	pusha
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov eax, esp
	push eax
	; Call the C kernel hardware interrupt handler
	mov eax, isr_handler
	call eax
	pop eax
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8
	sti
	iret

; extern 

extern isr_handler
idt_excp_stub:
	pusha
	push ds
	push es
	push fs
	push gs
	mov ax, 0x10
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov eax, esp
	push eax
	; Call the C kernel hardware interrupt handler
	mov eax, isr_handler
	call eax
	pop eax
	pop gs
	pop fs
	pop es
	pop ds
	popa
	add esp, 8
	sti
	iretd