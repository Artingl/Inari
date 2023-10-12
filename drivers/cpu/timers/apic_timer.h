#pragma once

#define APIC_TIMER_DIV              0xb

#define APIC_TIMER_LAST             0x38F
#define APIC_TIMER_DISABLE          0x10000
#define APIC_TIMER_SW_ENABLE        0x100
#define APIC_TIMER_CPUFOCUS         0x200
#define APIC_TIMER_NMI              (4<<8)
#define APIC_TIMER_TMR_PERIODIC	    0x20000
#define APIC_TIMER_TMR_BASEDIV	    (1<<20)

void cpu_atimer_init();
void cpu_atimer_disable();
