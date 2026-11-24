#ifndef BOOT_UI_H
#define BOOT_UI_H

#include "types.h"
#include "kbd.h"

#define MOD_ALT 0
#define MOD_WIN 1

void ui_init(void);
void ui_handle_key(KeyEvent ev);
void ui_tick_second(u32 uptime_sec);
u8   ui_get_mod_key(void);
u8   ui_is_configured(void);

#endif