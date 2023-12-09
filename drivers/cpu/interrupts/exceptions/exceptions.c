#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/interrupts/irq/irq.h>
#include <drivers/cpu/interrupts/exceptions/exceptions.h>

#include <drivers/memory/vmm.h>

void cpu_exceptions_core_init(struct cpu_core *core)
{
    // install all exceptions
    cpu_idt_install(core, (unsigned)_excp0, DIVISION_ERROR_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp1, DEBUG_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp2, NON_MASKABLE_INTERRUPT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp3, BREAKPOINT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp4, OVERFLOW_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp5, BOUND_RAGE_EXCEEDED_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp6, INVALID_OPCODE_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp7, DEVICE_NOT_AVAILABLE_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp8, DOUBLE_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp9, CROSS_SEGMENT_OVERRRUN_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp10, INVALID_TSS_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp11, SEGMENT_NOT_PRESENT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp12, STACK_SEGMENT_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp13, GENERAL_PROTECTION_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp14, PAGE_FAULT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp16, x87_FLOATING_POINT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp17, ALIGNMENT_CHECK_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp18, MACHINE_CHECK_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp19, SIMD_FLOATING_POINT_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp20, VIRTUALIZATION_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp21, CONTROL_PROTECTION_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp29, HYPERVISOR_INJECTION_EXCEPTION, 0x08, 0x8e);
    cpu_idt_install(core, (unsigned)_excp30, SECURITY_EXCEPTION, 0x08, 0x8e);
}

static inline void byte2str(char *buffer, uint8_t num)
{
    do
    {
        unsigned long temp;

        temp = (unsigned long)num % 16;
        buffer--;
        if (temp < 10)
            *buffer = temp + '0';
        else
            *buffer = temp - 10 + 'a';
        num = (unsigned long)num / 16;
    } while (num != 0);
}

void _invalid_opcode_exception_handler(struct regs32 *regs)
{
    uint8_t *i;
    size_t buffer_offset = 0;
    uint8_t offs = 0;
    char buffer[51];

    printk(KERN_DEBUG "The eIP at which the exception occurred: %p", (unsigned long)regs->eip);
    printk(KERN_DEBUG "64 bytes of memory dump from eIP-16:");

    memcpy(&buffer[0], "0x00: 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00\0", 51);
    for (i = (uint8_t *)regs->eip - 16; i < regs->eip + 64; i++)
    {
        byte2str(&buffer[10 + buffer_offset * 5], *i);

        offs++;
        buffer_offset++;

        if (buffer_offset % 9 == 0)
        {
            printk(KERN_DEBUG "\t%s", buffer);
            buffer_offset = 0;
            byte2str(&buffer[4], offs);
        }
    }
}

extern void kernel_loop();

extern int page_fault_handler(struct cpu_core *core, struct regs32 *r);

uintptr_t cpu_exceptions_core_handle(struct cpu_core *core, struct regs32 *regs)
{
    static int32_t counter = 0;

    if (counter == 0)
    {
        counter++;

        if (regs->int_no == PAGE_FAULT_EXCEPTION)
        {
            page_fault_handler(core, regs);
            // return 0;
        }

        printk(KERN_ERR "CPU Exception: %s", EXCEPTIONS_NAMES[regs->int_no]);
        printk(KERN_ERR "\tGS  = 0x%x, FS  = 0x%x, ES  = 0x%x", regs->gs, regs->fs, regs->es);
        printk(KERN_ERR "\tEDI = 0x%x, ESI = 0x%x, EBP = 0x%x", regs->edi, regs->esi, regs->ebp);
        printk(KERN_ERR "\tESP = 0x%x, EBX = 0x%x, EDX = 0x%x", regs->esp, regs->ebp, regs->edx);
        printk(KERN_ERR "\tECX = 0x%x, EAX = 0x%x, INT = 0x%x", regs->ecx, regs->eax, regs->int_no);
        printk(KERN_ERR "\tERR = 0x%x, EIP = 0x%x, CS  = 0x%x", regs->err_code, regs->eip, regs->cs);
        printk(KERN_ERR "\tEFLAGS = 0x%x, USERSP = 0x%x, SS = 0x%x", regs->eflags, regs->useresp, regs->ss);
        printk(KERN_ERR "");
        printk(KERN_ERR "\tEIP real: %p", (unsigned long)vmm_get_phys(vmm_current_directory(), regs->eip));
        // printk("\tTotal spurious interrupts 0x%x", cpu_irq_spurious_count());

        _invalid_opcode_exception_handler(regs);

        panic("-------------");
        counter--;
    }
    else {
        printk(KERN_ERR "!!! NESTED EXCEPTION");
    }

    while (1)
    {
        __asm__ volatile("cli\nhlt");
    }
}
