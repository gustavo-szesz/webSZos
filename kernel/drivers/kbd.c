// kernel/drivers/kbd.c
#include "kbd.h"
#include "io.h"

static u8 alt_held   = 0;
static u8 win_held   = 0;
static u8 ctrl_held  = 0;
static u8 shift_held = 0;
static u8 e0_prefix  = 0;

static const char scancode_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0,  ' ', 0
};

int kbd_poll(KeyEvent *ev) {
    u8 status = inb(0x64);
    if (!(status & 0x01) || (status & 0x20)) return 0;

    u8 sc = inb(0x60);

    if (sc == 0xE0) {
        e0_prefix = 1;
        return 0;
    }

    // Tecla solta (break)
    if (sc & 0x80) {
        u8 make = sc & 0x7F;
        if (e0_prefix) {
            if (make == 0x5B || make == 0x5C) win_held = 0;
            e0_prefix = 0;
        } else {
            if (make == 0x38) alt_held = 0;
            if (make == 0x1D) ctrl_held = 0;
            if (make == 0x2A || make == 0x36) shift_held = 0;
        }
        return 0;
    }

    ev->alt   = alt_held;
    ev->win   = win_held;
    ev->ctrl  = ctrl_held;
    ev->shift = shift_held;

    if (e0_prefix) {
        e0_prefix = 0;
        if (sc == 0x5B || sc == 0x5C) {
            win_held = 1;
            ev->win = 1;
            ev->code = KEY_WIN;
            return 1;
        }
        if (sc == 0x4B) { ev->code = KEY_LEFT;  return 1; }
        if (sc == 0x4D) { ev->code = KEY_RIGHT; return 1; }
        return 0;
    }

    if (sc == 0x38) { alt_held = 1;   return 0; }
    if (sc == 0x1D) { ctrl_held = 1;  return 0; }
    if (sc == 0x2A || sc == 0x36) { shift_held = 1; return 0; }

    if (sc < 128 && scancode_ascii[sc]) {
        ev->code = (u8)scancode_ascii[sc];
        return 1;
    }

    return 0;
}