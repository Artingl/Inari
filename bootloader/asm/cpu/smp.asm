section .text

global ap_trampoline
global ap_trampoline_end

extern gdt_descriptor 
extern lo_stack_bottom
extern lo_cpu_smp_ap_init

bits 16
ap_trampoline:
    cli
    cld

    lgdt [gdt_descriptor]
    mov eax, cr0
    or al, 1
    mov cr0, eax

    jmp 0x08:(0x8000 + (.sz - ap_trampoline))
.sz

bits 32
ap_protected:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;; get lapic id
    mov eax, 1
    cpuid
    shr ebx, 24
    mov edi, ebx

    ;; reuse lower stack for the AP based on lapic id (64 bytes of stack for each AP)
    mov esp, lo_stack_bottom
    mov eax, 64
    imul eax, edi
    add esp, eax

    cli
    cld

    push edi
    call 0x08:lo_cpu_smp_ap_init

ap_fail:
    ;; We should not get here...
    jmp $

ap_trampoline_end:
