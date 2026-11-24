#include "boot_ui.h"
#include "vbe.h"
#include "theme.h"

#define BAR_H 24

static u8 selected_mod = MOD_ALT; // 0 = ALT, 1 = WIN
static u8 confirmed    = 0;
static u32 cur_sec     = 0;

static void put_2d(char *dst, u32 val) {
    dst[0] = (char)('0' + ((val / 10) % 10));
    dst[1] = (char)('0' + (val % 10));
}

static void draw_bottom_status(void) {
    u32 by = SCREEN_H - BAR_H;
    draw_rect(0, by, SCREEN_W, BAR_H, COL_BG);
    draw_rect(0, by, SCREEN_W, 1, COL_BLUE); // Linha superior azul

    // Informações em verde claro
    draw_string(10,  by + 8, "IPv6: fe80::a8bb:ccff:fe12", COL_GREEN, COL_BG);
    draw_string(226, by + 8, "|", COL_BLUE, COL_BG);
    draw_string(242, by + 8, "IPv4: 192.168.1.42", COL_GREEN, COL_BG);
    draw_string(394, by + 8, "|", COL_BLUE, COL_BG);
    draw_string(410, by + 8, "SSD: 128GB", COL_GREEN, COL_BG);
    draw_string(506, by + 8, "|", COL_BLUE, COL_BG);
    draw_string(522, by + 8, "RAM: 4MB USED / 28MB FREE", COL_GREEN, COL_BG);
    draw_string(730, by + 8, "|", COL_BLUE, COL_BG);

    // Data e hora (2026-11-23 12:18:00 + uptime)
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

    // Janela preta com borda azul
    draw_rect(mx, my, mw, mh, COL_BG);
    draw_border(mx, my, mw, mh, 1, COL_BLUE);

    // Cabeçalho da janela
    draw_rect(mx + 1, my + 1, mw - 2, 22, COL_SURFACE);
    draw_rect(mx + 1, my + 23, mw - 2, 1, COL_BLUE);
    draw_string(mx + 12, my + 8, "WebSZos // Configuracao Inicial", COL_GREEN, COL_SURFACE);

    draw_string(mx + 24, my + 42, "Selecione a tecla modificadora ($mod):", COL_GREEN, COL_BG);

    // Botão ALT
    u32 alt_act = (selected_mod == MOD_ALT);
    u32 alt_bg  = alt_act ? COL_BLUE : COL_BG;
    u32 alt_fg  = alt_act ? COL_BG   : COL_GREEN;
    draw_rect(mx + 44, my + 74, 136, 36, alt_bg);
    draw_border(mx + 44, my + 74, 136, 36, 1, COL_BLUE);
    draw_string(mx + 84, my + 88, "[ ALT ]", alt_fg, alt_bg);

    // Botão WIN
    u32 win_act = (selected_mod == MOD_WIN);
    u32 win_bg  = win_act ? COL_BLUE : COL_BG;
    u32 win_fg  = win_act ? COL_BG   : COL_GREEN;
    draw_rect(mx + 220, my + 74, 136, 36, win_bg);
    draw_border(mx + 220, my + 74, 136, 36, 1, COL_BLUE);
    draw_string(mx + 260, my + 88, "[ WIN ]", win_fg, win_bg);

    // Dicas em verde claro apagado
    draw_string(mx + 34, my + 132, "Setas / Tab: Alternar  |  Enter: Confirmar", COL_GREEN_DIM, COL_BG);
    draw_string(mx + 50, my + 148, "(Ou pressione ALT / WIN diretamente)", COL_GREEN_DIM, COL_BG);
}

static void draw_workspace_ready(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H - BAR_H, COL_BG);

    // Janela inicial de confirmação com borda azul
    draw_rect(16, 16, SCREEN_W - 32, SCREEN_H - BAR_H - 32, COL_BG);
    draw_border(16, 16, SCREEN_W - 32, SCREEN_H - BAR_H - 32, 1, COL_BLUE);

    const char *mod_label = (selected_mod == MOD_WIN) ? "$mod = WIN (Super)" : "$mod = ALT";
    draw_string(32, 32, "WebSZos iniciado.", COL_GREEN, COL_BG);
    draw_string(32, 48, mod_label, COL_GREEN, COL_BG);
    draw_bottom_status();
}

static void render_all(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);
    if (!confirmed) {
        draw_mod_modal();
    } else {
        draw_workspace_ready();
    }
    draw_bottom_status();
}

void ui_init(void) {
    selected_mod = MOD_ALT;
    confirmed = 0;
    cur_sec = 0;
    render_all();
}

void ui_handle_key(KeyEvent ev) {
    if (!confirmed) {
        if (ev.alt) {
            selected_mod = MOD_ALT;
            confirmed = 1;
            render_all();
            return;
        }
        if (ev.win || ev.code == KEY_WIN) {
            selected_mod = MOD_WIN;
            confirmed = 1;
            render_all();
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
            confirmed = 1;
            render_all();
        }
    }
}

void ui_tick_second(u32 uptime_sec) {
    cur_sec = uptime_sec;
    draw_bottom_status();
}

u8 ui_get_mod_key(void) {
    return selected_mod;
}

u8 ui_is_configured(void) {
    return confirmed;
}