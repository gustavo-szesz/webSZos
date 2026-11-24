#include "idt.h"
#include "io.h"

typedef struct __attribute__((packed)) {
    u16 base_low;
    u16 sel;
    u8  always0;
    u8  flags;
    u16 base_high;
} IdtEntry;

typedef struct __attribute__((packed)) {
    u16 limit;
    u32 base;
} IdtPtr;

static IdtEntry idt[256];
static IdtPtr   idtp;
static volatile u32 ticks = 0;

extern void idt_flush(u32);
extern void irq0_stub(void);
extern void isr_default_stub(void);

static void idt_set_gate(u8 num, u32 base, u16 sel, u8 flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

static void pic_remap(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    outb(0x21, 0xFE);
    outb(0xA1, 0xFF);
}

void irq0_handler(void) {
    ticks++;
    outb(0x20, 0x20);
}

u32 pit_get_ticks(void) {
    return ticks;
}

void pit_init(u32 hz) {
    u32 divisor = 1193180 / hz;
    outb(0x43, 0x36);
    outb(0x40, (u8)(divisor & 0xFF));
    outb(0x40, (u8)((divisor >> 8) & 0xFF));
}

void idt_init(void) {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (u32)&idt;

    for (u32 i = 0; i < 256; i++) {
        idt_set_gate((u8)i, (u32)isr_default_stub, 0x08, 0x8E);
    }

    pic_remap();
    idt_set_gate(32, (u32)irq0_stub, 0x08, 0x8E);

    idt_flush((u32)&idtp);
    pit_init(100);
    __asm__ volatile ("sti");
}
