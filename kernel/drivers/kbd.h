// kernel/drivers/kbd.h
#ifndef KBD_H
#define KBD_H

#include "types.h"

#define KEY_NONE   0
#define KEY_LEFT   0x81
#define KEY_RIGHT  0x82
#define KEY_WIN    0x83

typedef struct {
    u8 code;
    u8 alt;
    u8 win;
    u8 ctrl;
    u8 shift;
} KeyEvent;

int kbd_poll(KeyEvent *ev);

#endif