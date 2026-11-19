#ifndef SERIAL_H
#define SERIAL_H
#include "types.h"
void serial_putc(char c);
void serial_puts(const char *s);
void serial_puthex(u32 v);
char serial_getc(void);
#endif
