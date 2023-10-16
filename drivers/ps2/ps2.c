#include <kernel/kernel.h>

#include <drivers/impl.h>

#include <drivers/ps2/ps2.h>
#include <drivers/cpu/interrupts/interrupts.h>

#include <kernel/sys/userio/keyboard.h>

uint16_t __ps2_key_map[] = {
    0, KB_ESCAPE, KB_1, KB_2, KB_3, KB_4, KB_5, KB_6, KB_7, KB_8, KB_9, KB_0,
    KB_MINUS, KB_EQUAL, KB_BACKSPACE, KB_TAB, KB_Q, KB_W, KB_E, KB_R, KB_T, KB_Y, KB_U,
    KB_I, KB_O, KB_P, KB_LEFT_BRACKET, KB_RIGHT_BRACKET, KB_ENTER, KB_LEFT_CONTROL, KB_A, KB_S, KB_D,
    KB_F, KB_G, KB_H, KB_J, KB_K, KB_L, KB_SEMICOLON, KB_APOSTROPHE, KB_GRAVE_ACCENT, KB_LEFT_SHIFT, KB_BACKSLASH,
    KB_Z, KB_X, KB_C, KB_V, KB_B, KB_N, KB_M, KB_COMMA, KB_PERIOD, KB_SLASH, KB_RIGHT_SHIFT,
    KB_KP_MULTIPLY, KB_LEFT_ALT, KB_SPACE, KB_CAPS_LOCK, KB_F1, KB_F2, KB_F3, KB_F4, KB_F5, KB_F6,
    KB_F7, KB_F8, KB_F9, KB_F10, KB_NUM_LOCK, KB_SCROLL_LOCK, KB_KP_7, KB_KP_8, KB_KP_9, KB_KP_SUBTRACT,
    KB_KP_4, KB_KP_5, KB_KP_6, KB_KP_ADD, KB_KP_1, KB_KP_2, KB_KP_3, KB_KP_0, KB_KP_DECIMAL,
    0, 0, 0, KB_F11, KB_F12, 0, 0, 0, 0};

interrupt_handler_t __ps2_handler(struct regs32 *regs);

void ps2_init()
{
    // uncomment line in the IRQ APIC remap
    printk(KERN_INFO "Loading PS2 driver");
    cpu_interrupts_subscribe(&__ps2_handler, INTERRUPT_PS2);
}

interrupt_handler_t __ps2_handler(struct regs32 *regs)
{
    uint8_t c = __inb(PS2_PORT);

    if (c >= 0x81)
    {
        // release key
        drv_kb_update_state(__ps2_key_map[c - 0x80], false);
    }
    else
    {
        // press key
        drv_kb_update_state(__ps2_key_map[c], true);
    }

    // send ACK to the keyboard
    // __outb(PS2_PORT, 0xFA);
}
