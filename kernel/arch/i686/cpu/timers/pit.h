#pragma once

#include <kernel/libc/typedefs.h>

#define PIT_DATA0 0x40 //  Channel 0 data port (read/write)
#define PIT_DATA1 0x41 //  Channel 1 data port (read/write)
#define PIT_DATA2 0x42 //  Channel 2 data port (read/write)
#define PIT_CM 0x43    //  Mode/Command register (write only, a read is ignored)

#define PIT_OCW_MASK_BINCOUNT 1   // 00000001
#define PIT_OCW_MASK_MODE 0xE     // 00001110
#define PIT_OCW_MASK_RL 0x30      // 00110000
#define PIT_OCW_MASK_COUNTER 0xC0 // 11000000

#define PIT_OCW_BINCOUNT_BINARY 0 // 0
#define PIT_OCW_BINCOUNT_BCD 1    // 1

#define PIT_OCW_MODE_TERMINALCOUNT 0   // 0000
#define PIT_OCW_MODE_ONESHOT 0x2       // 0010
#define PIT_OCW_MODE_RATEGEN 0x4       // 0100
#define PIT_OCW_MODE_SQUAREWAVEGEN 0x6 // 0110
#define PIT_OCW_MODE_SOFTWARETRIG 0x8  // 1000
#define PIT_OCW_MODE_HARDWARETRIG 0xA  // 1010

#define PIT_OCW_RL_LATCH 0      // 000000
#define PIT_OCW_RL_LSBONLY 0x10 // 010000
#define PIT_OCW_RL_MSBONLY 0x20 // 100000
#define PIT_OCW_RL_DATA 0x30    // 110000

#define PIT_OCW_COUNTER_0 0    // 00000000
#define PIT_OCW_COUNTER_1 0x40 // 01000000
#define PIT_OCW_COUNTER_2 0x80 // 10000000

#define PIT_FREQ 1193182.0f

void cpu_pit_init();
void cpu_pit_disable();

uint32_t cpu_pit_read();
