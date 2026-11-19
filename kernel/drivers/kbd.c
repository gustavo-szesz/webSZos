#include "kbd.h"
#include "io.h"

static u8 alt_held   = 0;
static u8 ctrl_held  = 0;
static u8 shift_held = 0;

static const char scancode_ascii[128] = {
    0,  27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,  'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0,  ' ', 0
};

static const char scancode_shift[128] = {
    0,  27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,  'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0,  ' ', 0
};

int kbd_poll(KeyEvent *ev) {
    u8 status = inb(0x64);
    if (!(status & 0x01) || (status & 0x20)) {
        return 0; // Sem dados ou dado vindo do mouse PS/2
    }

    u8 sc = inb(0x60);

    // Tecla solta (break code: bit 7 ativo)
    if (sc & 0x80) {
        u8 make = sc & 0x7F;
        if (make == 0x38) alt_held = 0;
        if (make == 0x1D) ctrl_held = 0;
        if (make == 0x2A || make == 0x36) shift_held = 0;
        return 0;
    }

    // Modificadores pressionados
    if (sc == 0x38) { alt_held = 1;   return 0; }
    if (sc == 0x1D) { ctrl_held = 1;  return 0; }
    if (sc == 0x2A || sc == 0x36) { shift_held = 1; return 0; }

    ev->alt   = alt_held;
    ev->ctrl  = ctrl_held;
    ev->shift = shift_held;

    if (sc == 0x3B) { ev->code = KEY_F1; return 1; }
    if (sc == 0x3C) { ev->code = KEY_F2; return 1; }
    if (sc == 0x3D) { ev->code = KEY_F3; return 1; }

    if (sc < 128) {
        char c = shift_held ? scancode_shift[sc] : scancode_ascii[sc];
        if (c) {
            ev->code = (u8)c;
            return 1;
        }
    }
    return 0;
}