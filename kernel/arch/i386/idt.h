#ifndef IDT_H
#define IDT_H

#include "types.h"

void idt_init(void);
void pit_init(u32 hz);
u32  pit_get_ticks(void);

#endif
