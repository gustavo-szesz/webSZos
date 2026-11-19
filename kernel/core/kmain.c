#include "types.h"
#include "theme.h"
#include "serial.h"
#include "vbe.h"
#include "kernel/arch/drivers/kdb.h"
#include "i3wm.h"

void kmain(void) {
    serial_puts("[webszos] boot ok: kernel modular i386\r\n");

    u32 lfb = vbe_init(SCREEN_W, SCREEN_H, 32);
    serial_puts("LFB addr: ");
    serial_puthex(lfb);
    serial_puts("\r\n");

    wm_init();

    KeyEvent ev;
    for (;;) {
        if (kbd_poll(&ev)) {
            wm_handle_key(ev);
        }
        __asm__ volatile ("pause");
    }
}