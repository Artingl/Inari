#include <kernel/kernel.h>
#include <kernel/machine.h>

#include <kernel/arch/i386/cpu/acpi/acpi.h>
#include <kernel/arch/i386/cpu/cpu.h>

void machine_reboot()
{
    cpu_shutdown();

    // try to reboot using ACPI
    if (!cpu_acpi_reboot())
    {
        // could not use ACPI for reboot, try to do a 8042 reset
        // uint8_t good = 0x02;
        // while (good & 0x02)
        //     good = __inb(0x64);
        // __outb(0x64, 0xFE);

        // We couldn't reboot, just halt
        printk("The system is going to be halted NOW!");
        // __halt();
    }
}

void machine_poweroff()
{
    cpu_shutdown();

    // try to poweroff using ACPI
    if (!cpu_acpi_poweroff())
    {
        // We couldn't poweroff, just halt
        printk("The system is going to be halted NOW!");
        // __halt();
    }
}

void machine_halt()
{
    cpu_shutdown();

    printk("The system is going to be halted NOW!");
    // __halt();
}
