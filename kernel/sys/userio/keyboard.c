#include <kernel/kernel.h>

#include <kernel/sys/userio/keyboard.h>
#include <kernel/sys/console/console.h>

#include <kernel/include/C/string.h>

bool __keyboard_state[0xfff];

void sys_drv_kb_init()
{
    memset(&__keyboard_state[0], 0, sizeof(__keyboard_state));
}

void sys_drv_kb_update_state(size_t offset, bool state)
{
    __keyboard_state[offset] = state;

    if (state)
    {
        sys_console_printc((char)offset);
        sys_console_flush();
    }
}

bool sys_drv_kb_pressed(uint16_t key)
{
    return __keyboard_state[key];
}
