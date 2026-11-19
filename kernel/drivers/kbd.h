#ifndef KBD_H
#define KBD_H

#include "types.h"

#define KEY_NONE   0
#define KEY_F1     0x81
#define KEY_F2     0x82
#define KEY_F3     0x83
#define KEY_LEFT   0x84
#define KEY_RIGHT  0x85

typedef struct {
    u8 code;      // Caractere ASCII ou KEY_* especial
    u8 alt;       // 1 se Left/Right Alt pressionado
    u8 ctrl;      // 1 se Ctrl pressionado
    u8 shift;     // 1 se Shift pressionado
} KeyEvent;

// Retorna 1 se houve tecla pressionada (make code), 0 se não há nada no buffer
int kbd_poll(KeyEvent *ev);

#endif