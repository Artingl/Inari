#pragma once

#include <kernel/include/C/typedefs.h>

void memory_init();
void memory_info();

// Do not allow PMM to use this region
int memory_forbid_region(uintptr_t origin, size_t size);
