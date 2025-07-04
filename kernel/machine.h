#pragma once

#include <kernel/core/sched/scheduler.h>

#define machine_reboot() scheduler_create_task(&__machine_reboot, NULL)
#define machine_poweroff() scheduler_create_task(&__machine_poweroff, NULL)
#define machine_halt() scheduler_create_task(&__machine_halt, NULL)

void __machine_reboot();
void __machine_poweroff();
void __machine_halt();
