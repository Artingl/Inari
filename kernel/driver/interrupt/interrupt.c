#include <kernel/driver/interrupt/interrupt.h>
#include <kernel/driver/memory/memory.h>
#include <kernel/list/dynlist.h>
#include <kernel/core/syscall/syscall.h>

dynlist_t interrupt_handlers = {0};

struct int_handler
{
    uint8_t int_no;
    interrupt_handler_t handler;
};

int kern_interrupts_init()
{
    return 0;
}

int kern_interrupts_install_handler(uint8_t int_no, interrupt_handler_t handler)
{
    // Allocate memory for the handler structure
    struct int_handler *s_handler = kmalloc(sizeof(struct int_handler));
    s_handler->int_no = int_no;
    s_handler->handler = handler;

    return dynlist_append(interrupt_handlers, s_handler);
}

int kern_interrupts_uninstall_handler(interrupt_handler_t handler)
{
    size_t i;
    struct int_handler *s_handler = NULL,  *s_found_handler;

    // Search for the handler inside the handlers list
    for (i = 0; i < dynlist_size(interrupt_handlers); i++)
    {
        s_handler = dynlist_get(interrupt_handlers, i, struct int_handler*);

        if (s_handler != NULL && s_handler->handler == handler)
        {
            // We found the handler
            s_found_handler = s_handler;
            break;
        }
    }

    // Handler not found
    if (s_found_handler == NULL || i > dynlist_size(interrupt_handlers))
        return 1;
    
    dynlist_remove(interrupt_handlers, i);

    // Don't forget to free up the memory used by the handler's structure
    kfree(s_found_handler);
    return 0;
}

extern void scheduler_reschedule(void *regs_ptr);
extern int scheduler_enabled_flag;
extern uint64_t kernel_uptime_ticks;

uint32_t kern_interrupts_syscall_handle(uint8_t id, uint32_t param0, uint32_t param1, uint32_t param2, void *regs_ptr)
{
    return kern_syscall_handle(id, param0, param1, param2, regs_ptr);
}

// This function must be called from within implementation of an isr in the currently booted architecture
void kern_interrupts_arch_handle(uint8_t int_no, void *regs_ptr)
{
    size_t i;
    struct int_handler *s_handler = NULL;

    // Send interrupt event to all handlers
    for (i = 0; i < dynlist_size(interrupt_handlers); i++)
    {
        s_handler = dynlist_get(interrupt_handlers, i, struct int_handler*);

        if (s_handler != NULL && s_handler->int_no == int_no)
        {
            s_handler->handler();
        }
    }

    if (int_no == KERN_INTERRUPT_TIMER)
    {
        kernel_uptime_ticks++;

        // Call the scheduler if it is enabled
        if (scheduler_enabled_flag)
        {
            scheduler_reschedule(regs_ptr);
        }
    }
}
