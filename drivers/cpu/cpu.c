#include <kernel/kernel.h>

#include <drivers/cpu/cpu.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/timers/pit.h>
#include <drivers/cpu/timers/apic_timer.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/interrupts/irq/irq.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/impl.h>

#include <kernel/include/C/io.h>
#include <kernel/include/C/string.h>

// ID 0 is always BSP
struct cpu_core cores[KERN_MAX_CORES];

uint32_t cpu_features_edx;
uint32_t cpu_features_ecx;

uintptr_t cpu_ioapic;

uint32_t cpu_max_count = 1;
uint32_t cpu_count;

double cpu_timer_ticks;

char cpu_vendor[13];
char cpu_model[49];

void cpu_bsp_init()
{
    memset(0, &cores, sizeof(cores));

    size_t i;

    cpu_timer_ticks = 0;
    cpu_ioapic = NULL;

    // get CPU vendor
    uint32_t eax, ebx, ecx, edx;
    __cpuid(0, &eax, &ebx, &ecx, &edx);
    memcpy(&cpu_vendor[0], &ebx, 4);
    memcpy(&cpu_vendor[4], &edx, 4);
    memcpy(&cpu_vendor[8], &ecx, 4);
    cpu_vendor[12] = 0;

    // get CPU model
    __cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004)
    {
        // the CPU does not support getting its model
        memcpy(cpu_model, "Undefined\0", 10);
    }
    else
    {
        memset(cpu_model, 0, 49);
        uint8_t offset = 0;

        for (i = 0; i < 3; i++)
        {
            __cpuid(0x80000002 + i, &eax, &ebx, &ecx, &edx);

            memcpy(&cpu_model[offset], &eax, 4);
            offset += 4;
            memcpy(&cpu_model[offset], &ebx, 4);
            offset += 4;
            memcpy(&cpu_model[offset], &ecx, 4);
            offset += 4;
            memcpy(&cpu_model[offset], &edx, 4);
            offset += 4;
        }
    }

    // get feature bits
    __cpuid(0, &eax, &ebx, &cpu_features_ecx, &cpu_features_edx);

    if (!(cpu_features_edx & CPU_FEATURE_EDX_FPU))
        panic("CPU does not support FPU");
    

    // initialize interrupts
    __disable_int();
    cpu_ints_init();

    // Init ACPI
    cpu_acpi_init();
    if (cpu_acpi_loaded())
    {
        cpu_count = cpu_acpi_load_madt(&cores);
    }
    else {
        cores[0].lapic_id = -1;
        cpu_count = 1;
    }

#ifdef CONFIG_CPU_NOAPIC
    cores[0].lapic_id = -1;
    cpu_count = 1;
#endif

    if (cpu_count > KERN_MAX_CORES)
        panic("Reached maximum amount of CPU cores (KERN_MAX_CORES)");

    // Init BSP core
    cpu_init_core(0);

    // TODO: initialize PIT anyway since we didn't implement apic timer yet
    cpu_pit_init();

    // show features info
    if (cpu_features_edx & CPU_FEATURE_EDX_SSE2)
        printk(KERN_DEBUG "CPU does support SSE2");
    if (cpu_features_edx & CPU_FEATURE_EDX_SSE)
        printk(KERN_DEBUG "CPU does support SSE");

    // Check if we're using apic
    if (cpu_using_apic())
    {
        // Init and IO/APIC and remap IRQs
        cpu_io_apic_init(cpu_ioapic_ptr());
        cpu_irq_apic_remap();
        
        // Bringup all other CPU cores
        if (cpu_count > 1)
        {
            cpu_max_count = cpu_count;
            cpu_count = 1;
            cpu_smp_bringup(cpu_max_count);
        }
    }

    // print info
    printk(KERN_INFO "CPU info:");
    printk(KERN_INFO "\tvendor: %s", cpu_vendor);
    printk(KERN_INFO "\tmodel: %s", cpu_model);
    printk(KERN_INFO "\tphysical_cpus: %d", cpu_count);
    printk(KERN_INFO "\tUsing APIC: %s", (cpu_using_apic() ? "yes" : "no"));
    printk(KERN_INFO "\tIs a VM: %s", (cpu_features_ecx & CPU_FEATURE_ECX_VMX ? "yes" : "no"));
    printk(KERN_INFO "\tfeatures (ecx): 0x%x", cpu_features_ecx);
    printk(KERN_INFO "\tfeatures (edx): 0x%x", cpu_features_edx);

    __enable_int();
}

void cpu_core_alloc(struct cpu_core *core)
{
    // Allocate IDT if it is null
    if (core->idt == NULL)
        core->idt = kmalloc(sizeof(struct cpu_idt) * 256);
}

void cpu_core_cleanup(struct cpu_core *core)
{
    // deallocate IDT if its allocated
    if (core->idt != NULL)
    {
        kfree(core->idt);
    }
}

void cpu_init_core(int id)
{
    struct cpu_core *core = &cores[id];

    cpu_core_alloc(core);

    if (id == 0)
        core->is_bsp = 1;

    // Initialize interrupts for the core
    cpu_ints_core_init(core);

    // ...
}

void cpu_idt_init(struct cpu_core *core)
{
    core->idt_desc.size = (sizeof(struct cpu_idt) * 256) - 1;
    core->idt_desc.base = (uintptr_t)core->idt;
    __load_idt(core->idt_desc);
}

void cpu_idt_install(struct cpu_core *core, unsigned long base, uint8_t num, uint16_t sel, uint8_t flags)
{
    core->idt[num].base_low = ((uint64_t)base & 0xFFFF);
    core->idt[num].base_high = ((uint64_t)base >> 16) & 0xFFFF;
    core->idt[num].sel = sel;
    core->idt[num].zero = 0;
    core->idt[num].flags = flags | 0x60;
}

void cpu_shutdown()
{
    __disable_int();
    size_t i;

    printk(KERN_NOTICE "Shutting down ALL CPUs!");

    if (cpu_using_apic())
    {
        cpu_io_apic_disable();
    }

    // disable interrupts
    for (i = 0; i < cpu_count; i++)
    {
        cpu_ints_core_disable(&cores[i]);
        cpu_core_cleanup(&cores[i]);
    }

    cpu_smp_shutdown();

    // halt or reboot if possible
    if (!cpu_acpi_reboot())
    {
        printk(KERN_NOTICE "The system is going to be halted NOW!");
        __halt();
    }
}

// we need to tell the compiler not to optimize the function, because then it will mess up while loop
__attribute__((optimize("O0")))
void cpu_sleep(size_t us)
{
    double start = kernel_uptime();
    double now = start;
    us = ((double)us) / 1000000.0f;

    while (start + us > now)
        now = kernel_uptime();
}

struct cpu_core *cpu_current_core()
{
    uint32_t _0, _1, ebx, _2;
    __cpuid(1, &_0, &ebx, &_1, &_2);
    return cpu_get_core(ebx << 24);
}

struct cpu_core *cpu_get_core(int id)
{
    if (cpu_max_count <= id)
    {
        panic("CPU id is more than available cores amount (%d <= %d)", cpu_max_count, id);
        return NULL;
    }

    cores[id].core_id = id;
    return &cores[id];
}

uint32_t cpu_cores_count()
{
    return cpu_count;
}

bool cpu_using_apic()
{
    #ifdef CONFIG_CPU_NOAPIC
        return 0;
    #endif
    
    return cpu_features_edx & CPU_FEATURE_EDX_APIC && !cpu_pic_is_enabled();
}

uint32_t cpu_feat_edx()
{
    return cpu_features_edx;
}

uint32_t cpu_feat_ecx()
{
    return cpu_features_ecx;
}

const char *cpu_get_vendor()
{
    return cpu_vendor;
}

const char *cpu_get_model()
{
    return cpu_model;
}

uint64_t cpu_timer_freq()
{
    return PIT_FREQ;
}

uintptr_t cpu_ioapic_ptr()
{
    return cpu_ioapic;
}
