#ifndef _INARI_ISM_INPUT_H
#define _INARI_ISM_INPUT_H

struct ism_keyboard {
    int released;
    uint8_t code;
    uint16_t key;
};

struct ism_mouse {
#define MOUSE_BTN1 0 // Left button
#define MOUSE_BTN2 1 // Right button
#define MOUSE_BTN3 2 // Middle button
    uint8_t buttons[8];  // Are any buttons pressed?

    int16_t pos_x;
    int16_t pos_y;

    int16_t rel_x;
    int16_t rel_y;

    int16_t wheel_rel_x;
    int16_t wheel_rel_y;
};

int input_read(struct ism_mouse *mouse, struct ism_keyboard *keyboard);

int input_init(void);
int input_cleanup(void);

#endif