#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/cpu/cpu.h>

#include <drivers/memory/memory.h>
#include <drivers/memory/vmm.h>

extern void ap_trampoline();
extern void ap_trampoline_end();
extern uint32_t cpu_count;

volatile uint32_t ap_flag;
volatile uint32_t ap_lock;

void cpu_smp_shutdown()
{
    printk(KERN_INFO "TODO: cpu_smp_shutdown");
}

void cpu_smp_bringup(int cores_count)
{
    size_t i, j;
    struct cpu_core *bsp_core = cpu_current_core();

    // Copy APs trampoline code to the 0x8000:0x0000, where all APs will start their execution
    size_t trampoline_size = (size_t)&ap_trampoline_end - (size_t)&ap_trampoline;
    memcpy((void *)0x8000, &ap_trampoline, trampoline_size);

    printk(KERN_DEBUG "smp: copied %d bytes of the APs trampoline code", trampoline_size);

#define WAIT_DELIVERY(addr, val)                \
    do                                          \
    {                                           \
        __asm__ volatile("pause" ::: "memory"); \
    } while (cpu_lapic_in(bsp_core, addr) & (val))

    // try to bring up all specified cores
    ap_lock = 1;
    for (i = 0; i < cores_count; i++)
    {
        struct cpu_core *ap_core = cpu_get_core(i);
        if (ap_core->lapic_id == bsp_core->lapic_id)
            continue;

        // Clear errors, select AP (core) and trigger INIT IPI
        cpu_lapic_out(bsp_core, LAPIC_ESR, 0);
        cpu_lapic_out(bsp_core, LAPIC_ICRHI, (cpu_lapic_in(bsp_core, LAPIC_ICRHI) & 0x00ffffff) | (i << 24));
        cpu_lapic_out(bsp_core, LAPIC_ICRLO, (cpu_lapic_in(bsp_core, LAPIC_ICRLO) & 0xfff00000) | 0xc500);
        WAIT_DELIVERY(LAPIC_ICRLO, i << 12);
        cpu_lapic_out(bsp_core, LAPIC_ICRHI, (cpu_lapic_in(bsp_core, LAPIC_ICRHI) & 0x00ffffff) | (i << 24));
        cpu_lapic_out(bsp_core, LAPIC_ICRLO, (cpu_lapic_in(bsp_core, LAPIC_ICRLO) & 0xfff00000) | 0x8500);
        WAIT_DELIVERY(LAPIC_ICRLO, i << 12);

        // Sleep 10ms
        cpu_sleep(10000);

        // Trigger STARTUP IPI
        ap_flag = 1;
        do
        {
            cpu_lapic_out(bsp_core, LAPIC_ESR, 0);
            cpu_lapic_out(bsp_core, LAPIC_ICRHI, (cpu_lapic_in(bsp_core, LAPIC_ICRHI) & 0x00ffffff) | (i << 24));
            cpu_lapic_out(bsp_core, LAPIC_ICRLO, (cpu_lapic_in(bsp_core, LAPIC_ICRLO) & 0xfff0f800) | 0x608);
            WAIT_DELIVERY(LAPIC_ICRLO, i << 12);

            cpu_sleep(1000);
        } while (ap_flag);
    }
    ap_lock = 0;

    // Wait for all cores to fully initialize
    int initialized;
    do
    {
        initialized = 1;
        for (i = 0; i < cores_count; i++) {
            if (cpu_get_core(i)->enabled)
                initialized++;
        }
    } while(initialized < cores_count);
}

// AP lower init function that should enable paging, initialize proper stack and jump to the main kernel at +3GB
__attribute__((section(".lo_text")))
 __attribute__ ((noreturn))
void lo_cpu_smp_ap_init(uint32_t _, uint32_t lapic_id)
{
    // Switch to the kernel directory that should be in the kernel's payload
    // Note: we can't use vmm_switch_directory since we don't have access to kernel memory yet
    extern struct kernel_payload payload;
    __asm__ volatile("mov %0, %%cr3" ::"r"(&payload.core_directory->tablesPhys));
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0"
                     : "=r"(cr0));
    cr0 |= 0x80000000;
    __asm__ volatile("mov %0, %%cr0" ::"r"(cr0));

    printk(KERN_DEBUG "smp: stack=%p, pd=%p", (unsigned long)cpu_current_core()->stackptr, (unsigned long)cpu_current_core()->pd);

    ap_flag = 0;
    cpu_count++;
    while (ap_lock)
        ;

    // Right now we share the same stack as other APs (64 bytes away from each other) so we need to change the esp
    __asm__ volatile(
        "mov %0, %%esp\n"
        "call .new_stack\n"
        ".new_stack:\n"
        :: "r"(cpu_current_core()->stackptr)
    );

    ap_kmain(cpu_current_core());

    while (1)
    {
        __asm__ volatile("pause":::"memory");
    }
}
