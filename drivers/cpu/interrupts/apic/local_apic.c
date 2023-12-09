#include <kernel/kernel.h>

#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/interrupts/irq/irq.h>

#include <drivers/impl.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/string.h>

int apic_initialize_pic = 0;

void cpu_lapic_init(struct cpu_core *core)
{
    // map lapic memory
    kident(core->lapic_ptr - PAGE_SIZE, PAGE_SIZE * 2, KERN_PAGE_RW);

    // enable all kinds of interrupts
    cpu_lapic_out(core, LAPIC_TPR, 0);

    // enable and disable PIC (we we haven't yet) so it will do all stuff we need
    if (!apic_initialize_pic) {
        apic_initialize_pic = 1;
        cpu_pic_apic_config();
    }

    // initialize the Local APIC
    cpu_lapic_set_base(core, core->lapic_ptr);
    cpu_lapic_out(core, LAPIC_SVR, 0x100 | 0xff);

    printk(KERN_INFO "LAPIC: core %d ptr: %p", core->lapic_id, (unsigned long)core->lapic_ptr);
}

uintptr_t cpu_lapic_get_base(struct cpu_core *core)
{
    // uint32_t eax, edx;
    // __get_msr(IA32_APIC_BASE, &eax, &edx);

    // return (eax & 0xfffff000);

    return core->lapic_ptr;
}

void cpu_lapic_set_base(struct cpu_core *core, uintptr_t base)
{
    uint32_t eax = (base & 0xfffff0000) | IA32_APIC_BASE_ENABLE, edx;
    __set_msr(IA32_APIC_BASE, eax, edx);
}

uint32_t cpu_lapic_in(struct cpu_core *core, uint32_t addr)
{
    return *((volatile uint32_t*)(core->lapic_ptr + addr));
}

void cpu_lapic_out(struct cpu_core *core, uint32_t addr, uint32_t value)
{
    *((volatile uint32_t*)(core->lapic_ptr + addr)) = value;
}

void cpu_lapic_disable(struct cpu_core *core)
{
    // todo...
}
