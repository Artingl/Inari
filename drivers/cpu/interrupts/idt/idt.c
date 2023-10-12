#include <kernel/kernel.h>

#include <drivers/cpu/cpu.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/interrupts/idt/idt.h>
#include <drivers/cpu/interrupts/irq/irq.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/cpu/interrupts/exceptions/exceptions.h>

#include <kernel/include/C/typedefs.h>
#include <kernel/include/C/string.h>

struct IDT_ptr idt_descriptor;
struct IDT idt[256];

struct int_subscriber subscribed_interrupts[256][32];

void cpu_idt_init()
{
    idt_descriptor.size = (sizeof(struct IDT) * 256) - 1;
    idt_descriptor.base = (uintptr_t)&idt;

    __cpu_idt_load();
}

void cpu_idt_init_memory()
{
    extern struct IDT_ptr idt_descriptor;
    extern struct IDT idt[256];

    extern struct int_subscriber subscribed_interrupts[256][32];

    memset(&idt, 0, sizeof(struct IDT) * 256);
    memset(&subscribed_interrupts, 0, sizeof(subscribed_interrupts));
}

void cpu_idt_install(unsigned long base, uint8_t num, uint16_t sel, uint8_t flags)
{
    idt[num].base_low = ((uint64_t)base & 0xFFFF);
    idt[num].base_high = ((uint64_t)base >> 16) & 0xFFFF;
    idt[num].sel = sel;
    idt[num].zero = 0;
    idt[num].flags = flags | 0x60;
}

int32_t __idt_subscribe(interrupt_handler_t ptr, uint8_t type)
{
    int32_t i = 0;
    while (subscribed_interrupts[type][i].in_use)
    {
        i++;
        if (i >= 32)
            i = 0;
    }

    subscribed_interrupts[type][i].in_use = true;
    subscribed_interrupts[type][i].handler = ptr;
    return i;
}

void __idt_unsubscribe(uint8_t type, int32_t id)
{
    subscribed_interrupts[type][id].in_use = false;
}
