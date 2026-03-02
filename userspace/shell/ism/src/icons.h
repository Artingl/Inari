#ifndef _INARI_CURSOR_H
#define _INARI_CURSOR_H

#define MOUSE_WIDTH  16
#define MOUSE_HEIGHT 16

#define T 0
#define B 0x01
#define W 0xFF
#define G 0xCC
#define D 0x66


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

__attribute__((unused)) static uint8_t btn_minimize[16][16] = {
    { W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, B, B, B, B, B, B, B, B, G, G, D, B },
    { W, G, G, G, B, B, B, B, B, B, B, B, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, D, D, D, D, D, D, D, D, D, D, D, D, D, D, B },
    { B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B }
};

__attribute__((unused)) static uint8_t btn_maximize[16][16] = {
    { W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, B, B, B, B, B, B, B, B, G, G, D, B },
    { W, G, G, G, B, B, B, B, B, B, B, B, G, G, D, B },
    { W, G, G, G, B, G, G, G, G, G, G, B, G, G, D, B },
    { W, G, G, G, B, G, G, G, G, G, G, B, G, G, D, B },
    { W, G, G, G, B, G, G, G, G, G, G, B, G, G, D, B },
    { W, G, G, G, B, G, G, G, G, G, G, B, G, G, D, B },
    { W, G, G, G, B, G, G, G, G, G, G, B, G, G, D, B },
    { W, G, G, G, B, G, G, G, G, G, G, B, G, G, D, B },
    { W, G, G, G, B, B, B, B, B, B, B, B, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, D, D, D, D, D, D, D, D, D, D, D, D, D, D, B },
    { B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B }
};

__attribute__((unused)) static uint8_t btn_close[16][16] = {
    { W, W, W, W, W, W, W, W, W, W, W, W, W, W, W, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, B, B, G, G, G, G, B, B, G, G, D, B },
    { W, G, G, G, G, B, B, G, G, B, B, G, G, G, D, B },
    { W, G, G, G, G, G, B, B, B, B, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, B, B, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, B, B, B, B, G, G, G, G, D, B },
    { W, G, G, G, G, B, B, G, G, B, B, G, G, G, D, B },
    { W, G, G, G, B, B, G, G, G, G, B, B, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, G, G, G, G, G, G, G, G, G, G, G, G, G, D, B },
    { W, D, D, D, D, D, D, D, D, D, D, D, D, D, D, B },
    { B, B, B, B, B, B, B, B, B, B, B, B, B, B, B, B }
};

#endif