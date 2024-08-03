#include <kernel/kernel.h>

#include <kernel/arch/i386/cpu/cpu.h>
#include <kernel/arch/i386/cpu/interrupts/pic/pic.h>
#include <kernel/arch/i386/cpu/timers/pit.h>
#include <kernel/arch/i386/cpu/timers/apic_timer.h>
#include <kernel/arch/i386/cpu/interrupts/apic/local_apic.h>
#include <kernel/arch/i386/cpu/interrupts/apic/io_apic.h>
#include <kernel/arch/i386/cpu/interrupts/irq/irq.h>
#include <kernel/arch/i386/cpu/acpi/acpi.h>
#include <kernel/arch/i386/cpu/interrupts/interrupts.h>
#include <kernel/arch/i386/impl.h>

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

static void cpu_core_alloc(struct cpu_core *core);
static void cpu_core_cleanup(struct cpu_core *core);

void cpu_bsp_init()
{
    size_t i;
    memset((void*)&cores[0], 0, sizeof(cores));
    for (i = 0; i < KERN_MAX_CORES; i++)
        cores[i].core_id = i;

    cpu_timer_ticks = 0;
    cpu_ioapic = (uintptr_t)NULL;

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
        cpu_count = cpu_acpi_load_madt(&cores[0]);
    }
    else
    {
        cores[0].lapic_id = -1;
        cpu_count = 1;
        cpu_max_count = 1;
    }

#ifdef CONFIG_CPU_NOAPIC
    cores[0].lapic_id = -1;
    cpu_count = 1;
    cpu_max_count = 1;
#endif

    if (cpu_count > KERN_MAX_CORES)
        panic("cpu: reached maximum amount of CPU cores (increase the value of KERN_MAX_CORES)");

    // Allocate all necessary things for the cores
    for (i = 0; i < cpu_count; i++)
        cpu_core_alloc(&cores[i]);

    if (cpu_using_apic())
    {
        // Init and IO/APIC and remap IRQs
        cpu_io_apic_init(cpu_ioapic_ptr());
        cpu_irq_apic_remap();
    }

    // Init BSP core
    cpu_init_core(0);

    // show features info
    if (cpu_features_edx & CPU_FEATURE_EDX_SSE2)
        printk("CPU does support SSE2");
    if (cpu_features_edx & CPU_FEATURE_EDX_SSE)
        printk("CPU does support SSE");

    // Bringup all other CPU cores if we're using apic
    if (cpu_using_apic() && cpu_count > 1)
    {
        cpu_max_count = cpu_count;
        cpu_count = 1;
#ifndef CONFIG_CPU_NO_SMP
        cpu_smp_bringup(cpu_max_count);
#endif
    }

    printk("cpu: vendor=%s, model=%s, phys_cpus=%d", cpu_vendor, cpu_model, cpu_count);
    printk("cpu: is_vm=%d, exc=0x%x, edx=0x%x", (cpu_features_ecx & CPU_FEATURE_ECX_VMX) ? 1 : 0, cpu_features_ecx, cpu_features_edx);
}

static void cpu_core_alloc(struct cpu_core *core)
{
    // Allocate IDT if it is null
    if (core->idt == NULL)
        core->idt = kmalloc(sizeof(struct cpu_idt) * 256);

    // Set other flags and allocate page directory
    if (core->core_id == 0)
    {
        extern void __kstack_bottom_bsp();

        core->stackptr = &__kstack_bottom_bsp;
        core->is_bsp = 1;
    }
    else
    {
        core->stackptr = kmalloc(KERN_STACK_SIZE);
    }

    core->enabled = 0;
}

static void cpu_core_cleanup(struct cpu_core *core)
{
    // deallocate IDT if its allocated
    if (core->idt != NULL)
    {
        kfree(core->idt);
    }

    if (core->core_id != 0)
    {
        if (core->stackptr != NULL)
            kfree(core->stackptr);
    }

    core->enabled = 0;
}

void cpu_init_core(int id)
{
    struct cpu_core *core = &cores[id];
    vmm_switch_directory(vmm_kernel_directory());

    // Initialize interrupts for the core
    cpu_ints_core_init(core);

    // ...
    core->enabled = 1;
    __enable_int();
}

void cpu_idt_init(struct cpu_core *core)
{
    core->idt_desc.size = (sizeof(struct cpu_idt) * 256) - 1;
    core->idt_desc.base = (uintptr_t)core->idt;
    printk("idt: base ptr %p", (unsigned long)core->idt);
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

    printk("cpu: shutting down all CPUs!");

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
}

// we need to tell the compiler not to optimize the function, because then it will mess up while loop
__attribute__((optimize("O0"))) void cpu_sleep(size_t us)
{
    double start = kernel_time();
    double now = start;
    us = ((double)us) / 1000000.0f;

    while (start + us > now)
        now = kernel_time();
}

struct cpu_core *cpu_current_core()
{
    uint32_t _0, _1, ebx, _2;
    __cpuid(1, &_0, &ebx, &_1, &_2);
    return cpu_get_core(ebx >> 24);
}

struct cpu_core *cpu_get_core(uint32_t id)
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
