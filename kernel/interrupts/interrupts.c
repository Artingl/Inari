#include <kernel/inari.h>
#include <kernel/interrupts/interrupts.h>
#include <kernel/interrupts/irq.h>
#include <kernel/interrupts/swi.h>

#include <arch/sys.h>

struct interrupt_frame *interrupt_frames_core[CONFIG_MAX_CORES];

struct interrupt_frame *interrupt_get_frame() {
    uint32_t core_id = core_id();
    if (!interrupt_frames_core[core_id])
        return (struct interrupt_frame *)NULL;
    return interrupt_frames_core[core_id];
}

void interrupts_trigger(uint32_t interrupt) { trigger_interrupt(interrupt); }

void interrupt_dispatch(struct interrupt_frame frame) {
    extern struct interrupt_frame *interrupt_frames_core[CONFIG_MAX_CORES];

    uint32_t core_id = core_id();
    // if (interrupt_frames_core[core_id])
    //     panic("irq: nested interrupt");

    interrupt_frames_core[core_id] = &frame;

    if (frame.int_no >> 12 == 2)
        swi_dispatch(frame);
    else
        irq_dispatch(frame);

    interrupt_frames_core[core_id] = (struct interrupt_frame *)NULL;
}
