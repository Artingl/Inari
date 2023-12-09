#include <kernel/kernel.h>

#include <kernel/sys/userio/keyboard.h>
#include <kernel/sys/console/console.h>

#include <kernel/include/C/string.h>

bool __keyboard_state[0xfff];

void sys_kb_init()
{
    memset(&__keyboard_state[0], 0, sizeof(__keyboard_state));
}

void drv_kb_update_state(size_t offset, bool state)
{
    __keyboard_state[offset] = state;

    if (state)
    {
        console_printc((char)offset);
        console_flush();
    }
}

bool drv_kb_pressed(uint16_t key)
{
    return __keyboard_state[key];
}
