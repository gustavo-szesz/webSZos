#include "boot_ui.h"
#include "vbe.h"
#include "theme.h"

#define BAR_H         24
#define GAP           4
#define MAX_WINDOWS   4
#define TELA_BOOT_MOD 0
#define TELA_DESKTOP  1

typedef enum {
    WIN_SHELL,
    WIN_MONITOR
} WindowType;

typedef struct {
    u16        id;
    WindowType type;
    u16        x;
    u16        y;
    u16        width;
    u16        height;
    char       buffer_lines[24][80];
    u32        cursor_x;
    u32        cursor_y;
} Window;

typedef struct {
    u8     actual_screen;
    u8     key_mod;
    Window windows[MAX_WINDOWS];
    u8     window_count;
    int    focus_idx;
} StateOS;

static StateOS os = {
    .actual_screen = TELA_BOOT_MOD,
    .key_mod       = MOD_ALT,
    .window_count  = 0,
    .focus_idx     = -1
};

static u8  selected_mod = MOD_ALT;
static u32 cur_sec      = 0;

static void put_2d(char *dst, u32 val) {
    dst[0] = (char)('0' + ((val / 10) % 10));
    dst[1] = (char)('0' + (val % 10));
}

static void draw_bottom_status(void) {
    u32 by = SCREEN_H - BAR_H;
    draw_rect(0, by, SCREEN_W, BAR_H, COL_BG);
    draw_rect(0, by, SCREEN_W, 1, COL_BLUE);

    draw_string(10,  by + 8, "IPv6: fe80::a8bb:ccff:fe12", COL_GREEN, COL_BG);
    draw_string(226, by + 8, "|", COL_BLUE, COL_BG);
    draw_string(242, by + 8, "IPv4: 192.168.1.42", COL_GREEN, COL_BG);
    draw_string(394, by + 8, "|", COL_BLUE, COL_BG);
    draw_string(410, by + 8, "SSD: 128GB", COL_GREEN, COL_BG);
    draw_string(506, by + 8, "|", COL_BLUE, COL_BG);
    draw_string(522, by + 8, "RAM: 4MB USED / 28MB FREE", COL_GREEN, COL_BG);
    draw_string(730, by + 8, "|", COL_BLUE, COL_BG);

    u32 total_s = (12 * 3600) + (18 * 60) + cur_sec;
    u32 hh = (total_s / 3600) % 24;
    u32 mm = (total_s / 60) % 60;
    u32 ss = total_s % 60;

    char dt[20] = "2026-11-23 00:00:00";
    put_2d(&dt[11], hh);
    put_2d(&dt[14], mm);
    put_2d(&dt[17], ss);

    draw_string(SCREEN_W - 162, by + 8, dt, COL_GREEN, COL_BG);
}

static void draw_mod_modal(void) {
    u32 mw = 400;
    u32 mh = 180;
    u32 mx = (SCREEN_W - mw) / 2;
    u32 my = (SCREEN_H - BAR_H - mh) / 2;

    draw_rect(mx, my, mw, mh, COL_BG);
    draw_border(mx, my, mw, mh, 1, COL_BLUE);

    draw_rect(mx + 1, my + 1, mw - 2, 22, COL_SURFACE);
    draw_rect(mx + 1, my + 23, mw - 2, 1, COL_BLUE);
    draw_string(mx + 12, my + 8, "WebSZos // Configuracao Inicial", COL_GREEN, COL_SURFACE);
    draw_string(mx + 24, my + 42, "Selecione a tecla modificadora ($mod):", COL_GREEN, COL_BG);

    u32 alt_act = (selected_mod == MOD_ALT);
    draw_rect(mx + 44, my + 74, 136, 36, alt_act ? COL_BLUE : COL_BG);
    draw_border(mx + 44, my + 74, 136, 36, 1, COL_BLUE);
    draw_string(mx + 84, my + 88, "[ ALT ]", alt_act ? COL_BG : COL_GREEN, alt_act ? COL_BLUE : COL_BG);

    u32 win_act = (selected_mod == MOD_WIN);
    draw_rect(mx + 220, my + 74, 136, 36, win_act ? COL_BLUE : COL_BG);
    draw_border(mx + 220, my + 74, 136, 36, 1, COL_BLUE);
    draw_string(mx + 260, my + 88, "[ WIN ]", win_act ? COL_BG : COL_GREEN, win_act ? COL_BLUE : COL_BG);

    draw_string(mx + 34, my + 132, "Setas / Tab: Alternar  |  Enter: Confirmar", COL_GREEN_DIM, COL_BG);
    draw_string(mx + 50, my + 148, "(Ou pressione ALT / WIN diretamente)", COL_GREEN_DIM, COL_BG);
}

static void recalculate_layout_tiling(void) {
    if (os.window_count == 0) return;

    u16 total_gaps = GAP * (os.window_count + 1);
    u16 col_w      = (SCREEN_W - total_gaps) / os.window_count;
    u16 win_h      = SCREEN_H - BAR_H - (2 * GAP);

    for (u8 i = 0; i < os.window_count; i++) {
        os.windows[i].x      = GAP + i * (col_w + GAP);
        os.windows[i].y      = GAP;
        os.windows[i].width  = col_w;
        os.windows[i].height = win_h;
    }
}

static void render_desktop(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H - BAR_H, COL_BG);

    if (os.window_count == 0) {
        const char *hint = (os.key_mod == MOD_WIN)
            ? "Desktop vazio. Pressione WIN + Enter para abrir um shell."
            : "Desktop vazio. Pressione ALT + Enter para abrir um shell.";
        draw_string(24, 24, hint, COL_GREEN_DIM, COL_BG);
        return;
    }

    for (u8 i = 0; i < os.window_count; i++) {
        Window *w = &os.windows[i];
        u8 is_focused = ((int)i == os.focus_idx);
        u32 border_col = is_focused ? COL_BLUE : COL_BLUE_DIM;

        draw_rect(w->x, w->y, w->width, w->height, COL_BG);
        draw_border(w->x, w->y, w->width, w->height, is_focused ? 2 : 1, border_col);

        // Barra de título da janela
        draw_rect(w->x + 2, w->y + 2, w->width - 4, 20, COL_SURFACE);
        draw_string(w->x + 10, w->y + 8, "shell // sh", COL_GREEN, COL_SURFACE);

        // Prompt inicial dentro da janela
        draw_string(w->x + 12, w->y + 34, "szesz@webszos> ", COL_GREEN, COL_BG);
        if (is_focused) {
            draw_rect(w->x + 132, w->y + 33, 8, 10, COL_GREEN);
        }
    }
}

static void render_all(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);
    if (os.actual_screen == TELA_BOOT_MOD) {
        draw_mod_modal();
    } else {
        render_desktop();
    }
    draw_bottom_status();
}

static void open_new_shell(void) {
    if (os.window_count >= MAX_WINDOWS) return;

    u8 idx = os.window_count;
    os.windows[idx].id       = idx + 1;
    os.windows[idx].type     = WIN_SHELL;
    os.windows[idx].cursor_x = 0;
    os.windows[idx].cursor_y = 0;

    os.window_count++;
    os.focus_idx = idx;
    recalculate_layout_tiling();
}

static void close_window(int idx) {
    if (idx < 0 || idx >= os.window_count) return;

    for (int i = idx; i < os.window_count - 1; i++) {
        os.windows[i] = os.windows[i + 1];
    }
    os.window_count--;

    if (os.window_count == 0) {
        os.focus_idx = -1;
    } else if (os.focus_idx >= os.window_count) {
        os.focus_idx = os.window_count - 1;
    }
    recalculate_layout_tiling();
}

static void confirm_mod_and_enter_desktop(u8 mod) {
    os.key_mod       = mod;
    os.actual_screen = TELA_DESKTOP;
    open_new_shell(); // Já entra no desktop com 1 shell aberto
    render_all();
}

static void choice_mod(KeyEvent ev) {
    if (ev.alt) {
        confirm_mod_and_enter_desktop(MOD_ALT);
        return;
    }
    if (ev.win || ev.code == KEY_WIN) {
        confirm_mod_and_enter_desktop(MOD_WIN);
        return;
    }

    if (ev.code == KEY_LEFT || ev.code == '1') {
        selected_mod = MOD_ALT;
        draw_mod_modal();
    } else if (ev.code == KEY_RIGHT || ev.code == '2') {
        selected_mod = MOD_WIN;
        draw_mod_modal();
    } else if (ev.code == '\t' || ev.code == 'h' || ev.code == 'l') {
        selected_mod = (selected_mod == MOD_ALT) ? MOD_WIN : MOD_ALT;
        draw_mod_modal();
    } else if (ev.code == '\n') {
        confirm_mod_and_enter_desktop(selected_mod);
    }
}

static void routing_event_desktop(KeyEvent event) {
    u8 mod_active = (os.key_mod == MOD_ALT) ? event.alt : event.win;

    if (mod_active) {
        if (event.code == '\n') {
            open_new_shell();
            render_desktop();
        } else if (event.code == 'q') {
            close_window(os.focus_idx);
            render_desktop();
        } else if (event.code == '\t' && os.window_count > 0) {
            os.focus_idx = (os.focus_idx + 1) % os.window_count;
            render_desktop();
        }
    }
}

void ui_init(void) {
    selected_mod     = MOD_ALT;
    os.actual_screen = TELA_BOOT_MOD;
    os.window_count  = 0;
    os.focus_idx     = -1;
    cur_sec          = 0;
    render_all();
}

void loop_principal(void) {
    KeyEvent event;
    if (kbd_poll(&event)) {
        if (os.actual_screen == TELA_BOOT_MOD) {
            choice_mod(event);
        } else {
            routing_event_desktop(event);
        }
    }
}

void ui_tick_second(u32 uptime_sec) {
    cur_sec = uptime_sec;
    draw_bottom_status();
}