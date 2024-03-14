#pragma once

#include <kernel/include/C/typedefs.h>

// unimplemented ---------------------
#define KB_RELEASE                0
#define KB_PRESS                  1
#define KB_REPEAT                 2


#define KB_MOD_SHIFT           0x0001
#define KB_MOD_CONTROL         0x0002
#define KB_MOD_ALT             0x0004
#define KB_MOD_SUPER           0x0008
#define KB_MOD_CAPS_LOCK       0x0010
#define KB_MOD_NUM_LOCK        0x0020
// -----------------------------------

// keyboard
#define KB_SPACE              32
#define KB_APOSTROPHE         39  /* ' */
#define KB_COMMA              44  /* , */
#define KB_MINUS              45  /* - */
#define KB_PERIOD             46  /* . */
#define KB_SLASH              47  /* / */
#define KB_0                  48
#define KB_1                  49
#define KB_2                  50
#define KB_3                  51
#define KB_4                  52
#define KB_5                  53
#define KB_6                  54
#define KB_7                  55
#define KB_8                  56
#define KB_9                  57
#define KB_SEMICOLON          59  /* ; */
#define KB_EQUAL              61  /* = */
#define KB_A                  65
#define KB_B                  66
#define KB_C                  67
#define KB_D                  68
#define KB_E                  69
#define KB_F                  70
#define KB_G                  71
#define KB_H                  72
#define KB_I                  73
#define KB_J                  74
#define KB_K                  75
#define KB_L                  76
#define KB_M                  77
#define KB_N                  78
#define KB_O                  79
#define KB_P                  80
#define KB_Q                  81
#define KB_R                  82
#define KB_S                  83
#define KB_T                  84
#define KB_U                  85
#define KB_V                  86
#define KB_W                  87
#define KB_X                  88
#define KB_Y                  89
#define KB_Z                  90
#define KB_LEFT_BRACKET       91  /* [ */
#define KB_BACKSLASH          92  /* \ */
#define KB_RIGHT_BRACKET      93  /* ] */
#define KB_GRAVE_ACCENT       96  /* ` */
#define KB_WORLD_1            161 /* non-US #1 */
#define KB_WORLD_2            162 /* non-US #2 */
#define KB_ESCAPE             256
#define KB_ENTER              257
#define KB_TAB                258
#define KB_BACKSPACE          259
#define KB_INSERT             260
#define KB_DELETE             261
#define KB_RIGHT              262
#define KB_LEFT               263
#define KB_DOWN               264
#define KB_UP                 265
#define KB_PAGE_UP            266
#define KB_PAGE_DOWN          267
#define KB_HOME               268
#define KB_END                269
#define KB_CAPS_LOCK          280
#define KB_SCROLL_LOCK        281
#define KB_NUM_LOCK           282
#define KB_PRINT_SCREEN       283
#define KB_PAUSE              284
#define KB_F1                 290
#define KB_F2                 291
#define KB_F3                 292
#define KB_F4                 293
#define KB_F5                 294
#define KB_F6                 295
#define KB_F7                 296
#define KB_F8                 297
#define KB_F9                 298
#define KB_F10                299
#define KB_F11                300
#define KB_F12                301
#define KB_F13                302
#define KB_F14                303
#define KB_F15                304
#define KB_F16                305
#define KB_F17                306
#define KB_F18                307
#define KB_F19                308
#define KB_F20                309
#define KB_F21                310
#define KB_F22                311
#define KB_F23                312
#define KB_F24                313
#define KB_F25                314
#define KB_KP_0               320
#define KB_KP_1               321
#define KB_KP_2               322
#define KB_KP_3               323
#define KB_KP_4               324
#define KB_KP_5               325
#define KB_KP_6               326
#define KB_KP_7               327
#define KB_KP_8               328
#define KB_KP_9               329
#define KB_KP_DECIMAL         330
#define KB_KP_DIVIDE          331
#define KB_KP_MULTIPLY        332
#define KB_KP_SUBTRACT        333
#define KB_KP_ADD             334
#define KB_KP_ENTER           335
#define KB_KP_EQUAL           336
#define KB_LEFT_SHIFT         340
#define KB_LEFT_CONTROL       341
#define KB_LEFT_ALT           342
#define KB_LEFT_SUPER         343
#define KB_RIGHT_SHIFT        344
#define KB_RIGHT_CONTROL      345
#define KB_RIGHT_ALT          346
#define KB_RIGHT_SUPER        347
#define KB_MENU               348
//

bool drv_kb_pressed(uint16_t key);
void sys_kb_init();

void drv_kb_update_state(size_t offset, bool state);
