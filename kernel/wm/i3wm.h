#ifndef I3WM_H
#define I3WM_H

#include "types.h"
#include "kbd.h"

void wm_init(void);
void wm_render(void);
void wm_focus_next(void);
void wm_focus_prev(void);
void wm_set_workspace(u32 ws);
void wm_handle_key(KeyEvent ev);

#endif