section .lo_text

vga_clear_screen:
    push eax
    mov eax, 0xb8000
.vga_cs_start:
    cmp eax, 0xb8000 + 4000
    jz .vga_cs_end

    mov byte [eax], 0x00
    inc eax
    jmp .vga_cs_start
.vga_cs_end:
    mov dword [CURSOR], 0xb8000
    pop eax
    ret

vga_print_message:
    push eax
    mov eax, _VGA_PREFIX
    call vga_add_message
    mov eax, [ESP - 8]
    call vga_add_message
    pop eax
    ret

vga_add_message:
    push ecx
    push ebx

    mov ecx, dword [CURSOR]

.vga_pm_start:
    mov bh, byte [eax]
    inc eax
    cmp bh, 0
    jz .vga_pm_end

    cmp bh, 10
    jz .vga_pm_nl

    mov byte [ecx], bh
    inc ecx
    inc dword [CURSOR]
    mov byte [ecx], 0x07
    inc ecx
    inc dword [CURSOR]
    jmp .vga_pm_start
.vga_pm_nl:
    push ebx
    ; inc eax
    sub ecx, 0xb8000
    mov ebx, 0

.vga_pm_mod:
    inc ebx
    sub ecx, 160
    cmp ecx, 160
    jg .vga_pm_mod

    imul ebx, 160
    mov ecx, ebx

    add ecx, 0xb8000
    mov dword [CURSOR], ecx

    pop ebx
    jmp .vga_pm_start
.vga_pm_end:
    pop ebx
    pop ecx
    ret

init_vga:
    call vga_clear_screen
    ret

CURSOR: dd 0
_VGA_PREFIX: db "[Inari/Early] ", 0

global _VGA_PREFIX
