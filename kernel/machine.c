#include <kernel/kernel.h>
#include <kernel/machine.h>
#include <kernel/scheduler/scheduler.h>

#include <drivers/cpu/acpi/acpi.h>
#include <drivers/cpu/cpu.h>

void machine_reboot()
{
    scheduler_shutdown();
    cpu_shutdown();

    // try to reboot using ACPI
    if (!cpu_acpi_reboot())
    {
        // could not use ACPI for reboot, try to do a 8042 reset
        uint8_t good = 0x02;
        while (good & 0x02)
            good = __inb(0x64);
        __outb(0x64, 0xFE);

        // We couldn't reboot, just halt
        printk(KERN_NOTICE "The system is going to be halted NOW!");
        __halt();
    }
}

void machine_poweroff()
{
    scheduler_shutdown();
    cpu_shutdown();

    // try to poweroff using ACPI
    if (!cpu_acpi_poweroff())
    {
        // We couldn't poweroff, just halt
        printk(KERN_NOTICE "The system is going to be halted NOW!");
        __halt();
    }
}

void machine_halt()
{
    scheduler_shutdown();
    cpu_shutdown();

    printk(KERN_NOTICE "The system is going to be halted NOW!");
    __halt();
}
