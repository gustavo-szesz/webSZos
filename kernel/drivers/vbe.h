#ifndef VBE_H
#define VBE_H
#include "types.h"
u32  vbe_init(u16 width, u16 height, u16 bpp);
void draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color);
void draw_border(u32 x, u32 y, u32 w, u32 h, u32 t, u32 color);
void draw_char(u32 x, u32 y, char c, u32 fg, u32 bg);
void draw_string(u32 x, u32 y, const char *s, u32 fg, u32 bg);
#endif
