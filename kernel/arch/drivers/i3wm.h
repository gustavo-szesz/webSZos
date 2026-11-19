#ifndef I3WM_H
#define I3WM_H

#include "types.h"

void wm_init(void);
void wm_render(void);
void wm_focus_next(void);
void wm_set_workspace(u32 ws);

#endif