#ifndef _INARI_CURSOR_H
#define _INARI_CURSOR_H

#define MOUSE_WIDTH  16
#define MOUSE_HEIGHT 16

#define T 0
#define B 0x01
#define W 0xff

__attribute__((unused)) static uint8_t cursor_hand[MOUSE_HEIGHT][MOUSE_WIDTH] = {
    { T, T, T, T, T, B, B, T, T, T, T, T, T, T, T, T },
    { T, T, T, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { T, T, T, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { T, T, T, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { T, T, T, T, B, W, W, B, B, B, T, T, T, T, T, T },
    { T, T, T, T, B, W, W, B, W, W, B, B, T, T, T, T },
    { T, T, T, T, B, W, W, B, W, W, B, W, B, B, T, T },
    { T, T, B, B, B, W, W, B, W, W, B, W, W, W, B, T },
    { T, B, W, W, B, W, W, W, W, W, W, W, W, W, B, T },
    { T, B, W, W, W, W, W, W, W, W, W, W, W, W, B, T },
    { T, T, B, W, W, W, W, W, W, W, W, W, W, B, T, T },
    { T, T, T, B, W, W, W, W, W, W, W, W, W, B, T, T },
    { T, T, T, B, W, W, W, W, W, W, W, W, W, B, T, T },
    { T, T, T, T, B, W, W, W, W, W, W, W, B, T, T, T },
    { T, T, T, T, T, B, W, W, W, W, W, W, B, T, T, T },
    { T, T, T, T, T, T, B, B, B, B, B, B, T, T, T, T }
};

__attribute__((unused)) static uint8_t cursor_hourglass[MOUSE_HEIGHT][MOUSE_WIDTH] = {
    { T, T, T, B, B, B, B, B, B, B, B, B, B, T, T, T },
    { T, T, T, B, W, W, W, W, W, W, W, W, B, T, T, T },
    { T, T, T, B, B, B, B, B, B, B, B, B, B, T, T, T },
    { T, T, T, T, B, W, W, W, W, W, W, B, T, T, T, T },
    { T, T, T, T, T, B, W, W, W, W, B, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, W, B, T, T, T, T, T, T },
    { T, T, T, T, T, T, T, B, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, T, B, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, W, B, T, T, T, T, T, T },
    { T, T, T, T, T, B, W, W, W, B, B, T, T, T, T, T },
    { T, T, T, T, B, W, W, W, W, W, W, B, T, T, T, T },
    { T, T, T, B, W, W, W, W, W, W, W, W, B, T, T, T },
    { T, T, T, B, B, B, B, B, B, B, B, B, B, T, T, T },
    { T, T, T, B, W, W, W, W, W, W, W, W, B, T, T, T },
    { T, T, T, B, B, B, B, B, B, B, B, B, B, T, T, T },
    { T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T }
};

__attribute__((unused)) static uint8_t cursor_ibeam[MOUSE_HEIGHT][MOUSE_WIDTH] = {
    { T, T, T, T, T, B, B, B, B, B, T, T, T, T, T, T },
    { T, T, T, T, T, B, W, W, W, B, T, T, T, T, T, T },
    { T, T, T, T, T, B, B, W, B, B, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, B, B, W, B, B, T, T, T, T, T, T },
    { T, T, T, T, T, B, W, W, W, B, T, T, T, T, T, T },
    { T, T, T, T, T, B, B, B, B, B, T, T, T, T, T, T }
};

__attribute__((unused)) static uint8_t cursor_normal[MOUSE_HEIGHT][MOUSE_WIDTH] = {
    { B, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, B, T, T, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, B, T, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, B, T, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, B, T, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, B, T, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, B, T, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, W, B, T, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, W, W, B, T, T, T, T, T, T, T },
    { B, W, W, W, W, W, W, W, W, B, T, T, T, T, T, T },
    { B, W, W, W, W, W, B, B, B, B, T, T, T, T, T, T },
    { B, W, W, B, W, W, B, T, T, T, T, T, T, T, T, T },
    { B, W, B, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { B, B, T, T, B, W, W, B, T, T, T, T, T, T, T, T },
    { T, T, T, T, T, B, W, W, B, T, T, T, T, T, T, T },
    { T, T, T, T, T, T, B, B, T, T, T, T, T, T, T, T }
};

#endif