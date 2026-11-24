#include "gdt.h"

typedef struct __attribute__((packed)) {
    u16 limit_low;
    u16 base_low;
    u8  base_middle;
    u8  access;
    u8  granularity;
    u8  base_high;
} GdtEntry;

typedef struct __attribute__((packed)) {
    u16 limit;
    u32 base;
} GdtPtr;

static GdtEntry gdt[3];
static GdtPtr   gdtp;

extern void gdt_flush(u32);

static void gdt_set_gate(int num, u32 base, u32 limit, u8 access, u8 gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    gdt[num].access      = access;
}

void gdt_init(void) {
    gdtp.limit = (sizeof(GdtEntry) * 3) - 1;
    gdtp.base  = (u32)&gdt;

    gdt_set_gate(0, 0, 0, 0, 0);                // Null segment
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code segment (0x08)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data segment (0x10)

    gdt_flush((u32)&gdtp);
}
