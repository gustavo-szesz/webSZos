#include "serial.h"
#include "io.h"
#define COM1 0x3F8

void serial_putc(char c) {
    for (u32 i = 0; i < 100000; i++) {
        if (inb(COM1 + 5) & 0x20) break;
    }
    outb(COM1, (u8)c);
}

void serial_puts(const char *s) { while (*s) serial_putc(*s++); }

void serial_puthex(u32 v) {
    const char *hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 28; i >= 0; i -= 4) serial_putc(hex[(v >> i) & 0xF]);
}

char serial_getc(void) {
    while (!(inb(COM1 + 5) & 0x01)) {}
    return (char)inb(COM1);
}
