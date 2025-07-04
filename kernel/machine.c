#include <kernel/kernel.h>
#include <kernel/machine.h>


// These functions must be implemented inside the memory driver of currently booted architecture
extern void arch_cleanup();
extern void arch_poweroff();
extern void arch_reboot();
extern void arch_halt();

void machine_reboot()
{
    printk("kernel: system is rebooting!!!");
    
    // scheduler_shutdown();
    arch_cleanup();
    arch_reboot();
}

void machine_poweroff()
{
    printk("kernel: system is powering down!!!");

    // scheduler_shutdown();
    arch_cleanup();
    arch_poweroff();
}

void machine_halt()
{
    printk("The system is going to be halted NOW!");

    // scheduler_shutdown();
    arch_cleanup();
    arch_halt();
}
