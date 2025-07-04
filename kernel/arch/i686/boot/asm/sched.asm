section .text
bits 32

global entrycode_a
global entrycode_b
entrycode_a:
    mov ah, 0

.a:
    mov byte [0xb8000], 0x22
    mov byte [0xb8001], ah
    add ah, 1
    jmp .a

entrycode_b:
    mov ah, 128

.a:
    mov byte [0xb8002], 0x22
    mov byte [0xb8003], ah
    add ah, 1
    jmp .a