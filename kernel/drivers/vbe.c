#include "vbe.h"
#include "io.h"
#include "theme.h"
#include "font8x8.h"

static u32 *fb = 0;

static void vbe_write(u16 index, u16 value) {
    outw(0x01CE, index);
    outw(0x01CF, value);
}

static u32 pci_find_lfb(void) {
    for (u32 dev = 0; dev < 32; dev++) {
        u32 addr = 0x80000000 | (dev << 11);
        outl(0x0CF8, addr);
        if (inl(0x0CFC) == 0x11111234) {
            outl(0x0CF8, addr | 0x10);
            return inl(0x0CFC) & 0xFFFFFFF0;
        }
    }
    return 0xE0000000;
}

u32 vbe_init(u16 width, u16 height, u16 bpp) {
    vbe_write(4, 0x00);
    vbe_write(1, width);
    vbe_write(2, height);
    vbe_write(3, bpp);
    vbe_write(4, 0x41);
    u32 lfb = pci_find_lfb();
    fb = (u32 *)lfb;
    return lfb;
}

void draw_rect(u32 x, u32 y, u32 w, u32 h, u32 color) {
    for (u32 row = 0; row < h; row++) {
        u32 py = y + row;
        if (py >= SCREEN_H) break;
        for (u32 col = 0; col < w; col++) {
            u32 px = x + col;
            if (px >= SCREEN_W) break;
            fb[py * SCREEN_W + px] = color;
        }
    }
}

void draw_border(u32 x, u32 y, u32 w, u32 h, u32 t, u32 color) {
    draw_rect(x, y, w, t, color);
    draw_rect(x, y + h - t, w, t, color);
    draw_rect(x, y, t, h, color);
    draw_rect(x + w - t, y, t, h, color);
}

void draw_char(u32 x, u32 y, char c, u32 fg, u32 bg) {
    if (c < 32 || c > 126) c = '?';
    const unsigned char *glyph = font8x8[c - 32];
    for (u32 row = 0; row < 8; row++) {
        u32 py = y + row;
        if (py >= SCREEN_H) break;
        unsigned char bits = glyph[row];
        for (u32 col = 0; col < 8; col++) {
            u32 px = x + col;
            if (px >= SCREEN_W) break;
            fb[py * SCREEN_W + px] = (bits & (1 << col)) ? fg : bg;
        }
    }
}

void draw_string(u32 x, u32 y, const char *s, u32 fg, u32 bg) {
    u32 cx = x, cy = y;
    while (*s) {
        if (*s == '\n') { cx = x; cy += 10; }
        else { draw_char(cx, cy, *s, fg, bg); cx += 8; }
        s++;
    }
}
