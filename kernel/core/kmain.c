#include "types.h"
#include "theme.h"
#include "serial.h"
#include "vbe.h"
#include "gdt.h"
#include "idt.h"
#include "boot_ui.h"

void kmain(void) {
    serial_puts("[webszos] boot ok\r\n");

    gdt_init();
    vbe_init(SCREEN_W, SCREEN_H, 32);

    ui_init();
    idt_init();

    u32 last_sec = 0;

    for (;;) {
        loop_principal();

        u32 now_sec = pit_get_ticks() / 100;
        if (now_sec != last_sec) {
            last_sec = now_sec;
            ui_tick_second(now_sec);
        }

        __asm__ volatile ("hlt");
    }
}