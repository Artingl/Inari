#include <kernel/kernel.h>

#include <drivers/cpu/cpu.h>
#include <drivers/cpu/interrupts/pic/pic.h>
#include <drivers/cpu/timers/pit.h>
#include <drivers/cpu/timers/apic_timer.h>
#include <drivers/cpu/interrupts/apic/local_apic.h>
#include <drivers/cpu/interrupts/apic/io_apic.h>
#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/interrupts/interrupts.h>
#include <drivers/impl.h>

#include <kernel/include/C/io.h>
#include <kernel/include/C/string.h>

uint32_t __cpu_features_edx;
uint32_t __cpu_features_ecx;

uint32_t __cpu_family = CPU_INVALID_VENDOR;

uint8_t __cpu_count;

double __timer_ticks;

char __cpu_vendor[13];
char __cpu_model[49];

void cpu_init()
{
    size_t i;

    __timer_ticks = 0;

    // get CPU vendor
    uint32_t eax, ebx, ecx, edx;
    __cpuid(0, &eax, &ebx, &ecx, &edx);
    memcpy(&__cpu_vendor[0], &ebx, 4);
    memcpy(&__cpu_vendor[4], &edx, 4);
    memcpy(&__cpu_vendor[8], &ecx, 4);
    __cpu_vendor[12] = 0;

    // get CPU model
    __cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax < 0x80000004)
    {
        // the CPU does not support getting its model
        memcpy(__cpu_model, "Undefined\0", 10);
    }
    else
    {
        memset(__cpu_model, 0, 49);
        uint8_t offset = 0;

        for (i = 0; i < 3; i++)
        {
            __cpuid(0x80000002 + i, &eax, &ebx, &ecx, &edx);

            memcpy(&__cpu_model[offset], &eax, 4);
            offset += 4;
            memcpy(&__cpu_model[offset], &ebx, 4);
            offset += 4;
            memcpy(&__cpu_model[offset], &ecx, 4);
            offset += 4;
            memcpy(&__cpu_model[offset], &edx, 4);
            offset += 4;
        }
    }

    // get feature bits
    __cpuid(0, &eax, &ebx, &__cpu_features_ecx, &__cpu_features_edx);

    // get cpu vendor
    if (strcmp(__cpu_vendor, "GenuineIntel") == 0)
    {
        __cpu_family = CPU_INTEL_VENDOR;
    }
    else if (strcmp(__cpu_vendor, "AuthenticAMD") == 0 || strcmp(__cpu_vendor, "AMDisbetter!") == 0)
    {
        __cpu_family = CPU_AMD_VENDOR;
    }

    if (!(__cpu_features_edx & CPU_FEATURE_EDX_FPU))
        panic("CPU does not support FPU");

    // initialize some other drivers
    cpu_acpi_init();
    cpu_interrupts_init();

    // show features info
    cpu_check_features();

    // print info
    printk(KERN_INFO "CPU info:");
    printk(KERN_INFO "\tvendor: %s", __cpu_vendor);
    printk(KERN_INFO "\tmodel: %s", __cpu_model);
    printk(KERN_INFO "\tfamily: 0x%x (%s)", __cpu_family, CPU_VENDOR_NAMES[__cpu_family]);
    printk(KERN_INFO "\tphysical_cpus: %d", __cpu_count);
#ifndef CONFIG_CPU_NOAPIC
    printk(KERN_INFO "\tAPIC: %d", cpu_using_apic());
#endif
    printk(KERN_INFO "\tfeatures (ecx): 0x%x", __cpu_features_ecx);
    printk(KERN_INFO "\tfeatures (edx): 0x%x", __cpu_features_edx);
}

void cpu_check_features()
{
    if (__cpu_features_edx & CPU_FEATURE_EDX_SSE2)
        printk(KERN_WARNING "CPU does support SSE2");
    if (__cpu_features_edx & CPU_FEATURE_EDX_SSE)
        printk(KERN_WARNING "CPU does support SSE");
}

void cpu_shutdown()
{
    printk(KERN_NOTICE "Shutting down ALL CPUs!");

    // disable interrupts
    cpu_interrupts_disable();

    // halt or reboot if possible
    if (!cpu_acpi_reboot())
    {
        printk(KERN_NOTICE "The system is going to be halted NOW!");
        __asm__ volatile("hlt");
    }
}

// we need to tell the compiler not to optimize the function, because then it will mess up while loop
void __attribute__((optimize("O0"))) cpu_sleep(size_t us)
{
    double start = kernel_uptime();
    double now = start;
    us = ((double)us) / 1000000.0f;

    while (start + us > now)
        now = kernel_uptime();
}

bool cpu_using_apic()
{
    return !cpu_pic_is_enabled();
}

uint32_t cpu_features_edx()
{
    return __cpu_features_edx;
}

uint32_t cpu_features_ecx()
{
    return __cpu_features_ecx;
}

const char *cpu_vendor()
{
    return cpu_vendor;
}

const char *cpu_model()
{
    return cpu_model;
}

uint32_t cpu_vendor_id()
{
    return __cpu_family;
}

uint64_t __cpu_timer_freq()
{
    return PIT_FREQ;
}
