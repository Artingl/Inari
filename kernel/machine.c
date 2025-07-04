#include <kernel/kernel.h>
#include <kernel/machine.h>


// These functions must be implemented inside the memory driver of currently booted architecture
extern void arch_cleanup();
extern void arch_poweroff();
extern void arch_reboot();
extern void arch_halt();
extern void kern_shutdown();

void __machine_reboot()
{
    kern_shutdown();
    scheduler_shutdown();
    arch_cleanup();
    arch_reboot();
}

void __machine_poweroff()
{
    kern_shutdown();
    scheduler_shutdown();
    arch_cleanup();
    arch_poweroff();
}

void __machine_halt()
{
    kern_shutdown();
    scheduler_shutdown();
    arch_cleanup();
    arch_halt();
}
