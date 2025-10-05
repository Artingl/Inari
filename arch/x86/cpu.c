#include <kernel/inari.h>
#include <kernel/mm/kmalloc.h>

#include <misc/string.h>

#include <arch/x86/acpi.h>
#include <arch/x86/pic.h>
#include <arch/x86/irq.h>
#include <arch/x86/exception.h>
#include <arch/x86/cpu.h>
#include <arch/x86/arch.h>
#include <arch/sys.h>

static uint32_t features_ecx, features_edx;

static char s_cpu_vendor[16];
static char s_cpu_model[64];
static uint32_t phys_cpus;

struct x86_cpu x86_cpus_pool[CONFIG_MAX_CORES];

static void cpu_arch_core_idt_init(struct x86_cpu *core)
{
    core->idt_desc.size = (sizeof(struct x86_cpu_idt) * 256) - 1;
    core->idt_desc.base = (uintptr_t)core->idt;
}

static void cpu_core_init(struct x86_cpu *core)
{
    if (!core->is_available)
        return;
    core->idt = kmalloc(sizeof(struct x86_cpu_idt) * 256);
    cpu_arch_core_idt_init(core);

    x86_exception_setup(core);
    x86_irq_setup(core);

    __asm__ volatile("lidt %0" :: "m"(core->idt_desc));
}

int x86_cpu_init(void)
{
    size_t i;
    uint8_t offset = 0;
    uint32_t eax, ebx, ecx, edx;

    memset(&x86_cpus_pool[0], 0, sizeof(x86_cpus_pool));

    x86_cpuid(0, &eax, &ebx, &features_ecx, &features_edx);

    x86_cpuid(0, &eax, &ebx, &ecx, &edx);
    memcpy(&s_cpu_vendor[0], &ebx, 4);
    memcpy(&s_cpu_vendor[4], &edx, 4);
    memcpy(&s_cpu_vendor[8], &ecx, 4);
    s_cpu_vendor[12] = 0;

    x86_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004)
    {
        memcpy(s_cpu_model, "Undefined\0", 10);
    }
    else
    {
        for (i = 0; i < 3; i++)
        {
            x86_cpuid(0x80000002 + i, &eax, &ebx, &ecx, &edx);

            memcpy(&s_cpu_model[offset], &eax, 4);
            offset += 4;
            memcpy(&s_cpu_model[offset], &ebx, 4);
            offset += 4;
            memcpy(&s_cpu_model[offset], &ecx, 4);
            offset += 4;
            memcpy(&s_cpu_model[offset], &edx, 4);
            offset += 4;
        }

        s_cpu_model[offset-3] = '\0';
    }

    printk("cpu: vendor=%s; model=%s", s_cpu_vendor, s_cpu_model);

    if (x86_acpi_init() == 0)
    {
        x86_acpi_load_madt(&x86_cpus_pool[0]);
        phys_cpus = x86_acpi_cpu_count();
    }
    else phys_cpus = 1;

    printk("cpu: available %u core(s)", phys_cpus);

    for (i = 0; i < phys_cpus; i++)
        cpu_core_init(&x86_cpus_pool[i]);
    
    if (x86_pic_init())
        panic("cpu: cannot continue without pic.");

    enable_int();
    return 0;
}

void x86_cpu_acknowledge_irq(uint8_t irq)
{
    x86_pic_acknowledge(irq);
}

uint32_t x86_cpu_features_ecx(void)
{
    return features_ecx;
}

uint32_t x86_cpu_features_edx(void)
{
    return features_edx;
}

const char *x86_cpu_vendor(void)
{
    return (const char *)&s_cpu_vendor[0];
}

const char *x86_cpu_model(void)
{
    return (const char *)&s_cpu_model[0];
}

