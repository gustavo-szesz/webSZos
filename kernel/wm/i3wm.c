#include "i3wm.h"
#include "vbe.h"
#include "theme.h"
#include "serial.h"

typedef struct {
    u32 x, y, w, h;
    const char *title;
    const char *tag;
} Tile;

#define MODE_NORMAL 0
#define MODE_INSERT 1

static u32 current_ws = 0;
static u32 focused_tile = 0;
static u32 editor_mode = MODE_NORMAL;

// Terminal state
static char term_input[64];
static u32  term_len = 0;
static const char *term_output = "Commands: help | ls | mem | save | clear";

// Editor state (buffer real)
#define ED_MAX 1024
static char ed_buf[ED_MAX];
static u32  ed_len = 0;
static u32  ed_cur = 0;

// Scripts state
static const char *script_status = "Status: idle (F1..F3 ready)";

#define BAR_H   24
#define GAP     10
#define WORK_Y  (BAR_H + GAP)
#define WORK_H  (SCREEN_H - (BAR_H * 2) - (GAP * 2))
#define LEFT_W  ((SCREEN_W * 60) / 100)
#define RIGHT_X (GAP + LEFT_W + GAP)
#define RIGHT_W (SCREEN_W - RIGHT_X - GAP)
#define RIGHT_H ((WORK_H - GAP) / 2)

static Tile tiles[3] = {
    { GAP,     WORK_Y,                 LEFT_W,  WORK_H,  "terminal // fish",  "TTY0" },
    { RIGHT_X, WORK_Y,                 RIGHT_W, RIGHT_H, "nvim // scratch.c", "C99"  },
    { RIGHT_X, WORK_Y + RIGHT_H + GAP, RIGHT_W, RIGHT_H, "runner // scripts", "SH"   }
};

static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void ed_load_default(void) {
    const char *init = "#include \"webszos.h\"\n\nint main(void) {\n    return 0;\n}";
    ed_len = 0;
    while (init[ed_len] && ed_len < ED_MAX - 1) {
        ed_buf[ed_len] = init[ed_len];
        ed_len++;
    }
    ed_buf[ed_len] = 0;
    ed_cur = 0;
}

static void ed_insert_char(char c) {
    if (ed_len >= ED_MAX - 1) return;
    for (u32 i = ed_len; i > ed_cur; i--) {
        ed_buf[i] = ed_buf[i - 1];
    }
    ed_buf[ed_cur++] = c;
    ed_buf[++ed_len] = 0;
}

static void ed_delete_at(u32 pos) {
    if (pos >= ed_len) return;
    for (u32 i = pos; i < ed_len - 1; i++) {
        ed_buf[i] = ed_buf[i + 1];
    }
    ed_buf[--ed_len] = 0;
    if (ed_cur > ed_len) ed_cur = ed_len;
}

static void ed_save_to_host(void) {
    serial_puts("@@SAVE:scratch.c:");
    for (u32 i = 0; i < ed_len; i++) {
        if (ed_buf[i] == '\n') serial_puts("\\n");
        else serial_putc(ed_buf[i]);
    }
    serial_puts("\r\n");
    script_status = "Saved scratch.c -> localStorage!";
}

static void exec_term_cmd(void) {
    term_input[term_len] = 0;
    if (term_len == 0) {
        term_output = "";
    } else if (streq(term_input, "help")) {
        term_output = "help | ls | mem | edit | save | clear";
    } else if (streq(term_input, "ls")) {
        term_output = "scratch.c   build.sh   meminfo.sh   export.sh";
    } else if (streq(term_input, "mem")) {
        term_output = "RAM: 32MB | Kernel @ 0x100000 | LFB @ 0xE0000000";
    } else if (streq(term_input, "edit")) {
        focused_tile = 1;
        editor_mode = MODE_INSERT;
        term_output = "Switched to nvim // scratch.c (INSERT)";
    } else if (streq(term_input, "save")) {
        ed_save_to_host();
        term_output = "scratch.c exported via COM1 -> browser";
    } else if (streq(term_input, "clear")) {
        term_output = "";
    } else {
        term_output = "fish: unknown command (try 'help')";
    }
    term_len = 0;
    term_input[0] = 0;
}

void wm_init(void) {
    current_ws = 0;
    focused_tile = 0;
    editor_mode = MODE_NORMAL;
    term_len = 0;
    term_input[0] = 0;
    ed_load_default();
    wm_render();
}

void wm_focus_next(void) {
    focused_tile = (focused_tile + 1) % 3;
    editor_mode = MODE_NORMAL;
    wm_render();
}

void wm_focus_prev(void) {
    focused_tile = (focused_tile + 2) % 3;
    editor_mode = MODE_NORMAL;
    wm_render();
}

void wm_set_workspace(u32 ws) {
    if (ws < 3) {
        current_ws = ws;
        wm_render();
    }
}

void wm_handle_key(KeyEvent ev) {
    // Atalhos globais do i3wm (Alt + tecla) sempre têm prioridade
    if (ev.alt) {
        if (ev.code == 'l' || ev.code == 'j') { wm_focus_next(); return; }
        if (ev.code == 'h' || ev.code == 'k') { wm_focus_prev(); return; }
        if (ev.code == '1') { wm_set_workspace(0); return; }
        if (ev.code == '2') { wm_set_workspace(1); return; }
        if (ev.code == '3') { wm_set_workspace(2); return; }
    }

    // Scripts F1..F3
    if (ev.code == KEY_F1) {
        script_status = "build.sh -> [OK] 0 errors, 0 warnings";
        serial_puts("@@SCRIPT:build.sh\r\n");
        wm_render();
        return;
    }
    if (ev.code == KEY_F2) {
        script_status = "meminfo.sh -> 31.8 MB free / 32.0 MB";
        serial_puts("@@SCRIPT:meminfo.sh\r\n");
        wm_render();
        return;
    }
    if (ev.code == KEY_F3) {
        ed_save_to_host();
        wm_render();
        return;
    }

    // Ctrl+S salva de qualquer lugar
    if (ev.ctrl && (ev.code == 's' || ev.code == 'S')) {
        ed_save_to_host();
        wm_render();
        return;
    }

    // Tile 0: Terminal
    if (focused_tile == 0) {
        if (ev.code == '\t') { wm_focus_next(); return; }
        if (ev.code == '\n') {
            exec_term_cmd();
            wm_render();
        } else if (ev.code == '\b') {
            if (term_len > 0) {
                term_input[--term_len] = 0;
                wm_render();
            }
        } else if (ev.code >= 32 && ev.code <= 126 && term_len < sizeof(term_input) - 1) {
            term_input[term_len++] = (char)ev.code;
            term_input[term_len] = 0;
            wm_render();
        }
        return;
    }

    // Tile 1: Editor Modal (Neovim-like)
    if (focused_tile == 1) {
        if (editor_mode == MODE_NORMAL) {
            if (ev.code == '\t') { wm_focus_next(); return; }
            if (ev.code == 'i')  { editor_mode = MODE_INSERT; wm_render(); return; }
            if (ev.code == 'a')  {
                if (ed_cur < ed_len) ed_cur++;
                editor_mode = MODE_INSERT;
                wm_render();
                return;
            }
            if (ev.code == 'h' && ed_cur > 0)      { ed_cur--; wm_render(); return; }
            if (ev.code == 'l' && ed_cur < ed_len) { ed_cur++; wm_render(); return; }
            if (ev.code == 'x')                    { ed_delete_at(ed_cur); wm_render(); return; }
            if (ev.code == 'w')                    { ed_save_to_host(); wm_render(); return; }
        } else {
            // MODE_INSERT
            if (ev.code == 27) { // ESC
                editor_mode = MODE_NORMAL;
                wm_render();
                return;
            }
            if (ev.code == '\b') {
                if (ed_cur > 0) {
                    ed_cur--;
                    ed_delete_at(ed_cur);
                    wm_render();
                }
                return;
            }
            if (ev.code == '\t') {
                ed_insert_char(' ');
                ed_insert_char(' ');
                wm_render();
                return;
            }
            if (ev.code == '\n' || (ev.code >= 32 && ev.code <= 126)) {
                ed_insert_char((char)ev.code);
                wm_render();
                return;
            }
        }
        return;
    }

    // Tile 2: Scripts
    if (focused_tile == 2) {
        if (ev.code == '\t') { wm_focus_next(); return; }
    }
}

static void draw_topbar(void) {
    draw_rect(0, 0, SCREEN_W, BAR_H, COL_BAR);
    draw_rect(0, BAR_H - 1, SCREEN_W, 1, COL_BORDER_IN);

    const char *ws_names[3] = { " 1:term ", " 2:edit ", " 3:sys  " };
    for (u32 i = 0; i < 3; i++) {
        u32 bx = 6 + i * 70;
        u32 is_act = (i == current_ws);
        u32 bg = is_act ? COL_SURFACE : COL_BAR;
        u32 fg = is_act ? COL_ACCENT  : COL_SUBTEXT;

        draw_rect(bx, 2, 64, 20, bg);
        draw_string(bx, 8, ws_names[i], fg, bg);
        if (is_act) draw_rect(bx, 20, 64, 2, COL_ACCENT);
    }

    draw_string((SCREEN_W / 2) - 88, 8, "~/dev/webszos  >  i3wm", COL_SUBTEXT, COL_BAR);

    u32 rx = SCREEN_W - 266;
    draw_rect(rx, 3, 76, 18, COL_SURFACE);
    draw_string(rx + 6, 8, "CPU:i386", COL_MAUVE, COL_SURFACE);

    draw_rect(rx + 82, 3, 76, 18, COL_SURFACE);
    draw_string(rx + 88, 8, "MEM:32MB", COL_TEAL, COL_SURFACE);

    draw_rect(rx + 164, 3, 94, 18, COL_SURFACE);
    draw_string(rx + 170, 8, "1024x768x32", COL_ACCENT, COL_SURFACE);
}

static void draw_tile_frame(u32 idx) {
    Tile t = tiles[idx];
    u32 is_foc = (idx == focused_tile);
    u32 body_bg = is_foc ? COL_TILE_ACT : COL_SURFACE;
    u32 head_bg = is_foc ? COL_HEADER   : COL_BAR;
    u32 border  = is_foc ? COL_ACCENT   : COL_BORDER_IN;
    u32 title_c = is_foc ? COL_TEXT     : COL_SUBTEXT;

    draw_rect(t.x, t.y, t.w, t.h, body_bg);
    draw_border(t.x, t.y, t.w, t.h, is_foc ? 2 : 1, border);

    u32 pad = is_foc ? 2 : 1;
    draw_rect(t.x + pad, t.y + pad, t.w - pad * 2, 20, head_bg);
    draw_string(t.x + 10, t.y + 7, t.title, title_c, head_bg);

    u32 tag_bg = is_foc ? COL_ACCENT : COL_SURFACE;
    u32 tag_fg = is_foc ? COL_BG     : COL_SUBTEXT;
    draw_rect(t.x + t.w - 46, t.y + 4, 38, 14, tag_bg);
    draw_string(t.x + t.w - 42, t.y + 7, t.tag, tag_fg, tag_bg);
}

static void draw_editor_content(void) {
    u32 bg1 = (focused_tile == 1) ? COL_TILE_ACT : COL_SURFACE;
    u32 x1  = tiles[1].x + 2;
    u32 y1  = tiles[1].y + 22;

    // Gutter lateral
    draw_rect(x1, y1, 28, tiles[1].h - 24, COL_GUTTER);

    u32 cx = x1 + 36;
    u32 cy = y1 + 8;
    u32 line = 1;
    draw_char(x1 + 10, cy, '1', COL_SUBTEXT, COL_GUTTER);

    for (u32 i = 0; i <= ed_len; i++) {
        // Desenha cursor na posição ed_cur
        if (i == ed_cur && focused_tile == 1) {
            u32 cur_col = (editor_mode == MODE_INSERT) ? COL_GREEN : COL_ACCENT;
            char ch = (i < ed_len && ed_buf[i] != '\n') ? ed_buf[i] : ' ';
            draw_char(cx, cy, ch, COL_BG, cur_col);
        } else if (i < ed_len && ed_buf[i] != '\n') {
            draw_char(cx, cy, ed_buf[i], COL_TEXT, bg1);
        }

        if (i < ed_len) {
            if (ed_buf[i] == '\n') {
                cx = x1 + 36;
                cy += 10;
                line++;
                if (line < 10) {
                    draw_char(x1 + 10, cy, (char)('0' + line), COL_SUBTEXT, COL_GUTTER);
                }
            } else {
                cx += 8;
            }
        }
    }
}

static void draw_tiles_content(void) {
    // Tile 0: Terminal Interativo
    u32 bg0 = (focused_tile == 0) ? COL_TILE_ACT : COL_SURFACE;
    u32 x0 = tiles[0].x + 12;
    u32 y0 = tiles[0].y + 30;

    draw_string(x0, y0,      "[OK]", COL_GREEN, bg0);
    draw_string(x0 + 40, y0, "Kernel i386 freestanding booted", COL_TEXT, bg0);

    draw_string(x0, y0 + 14, "[OK]", COL_GREEN, bg0);
    draw_string(x0 + 40, y0 + 14, "PS/2 Keyboard + Modal Editor active", COL_TEXT, bg0);

    draw_rect(x0, y0 + 32, tiles[0].w - 24, 1, COL_BORDER_IN);

    draw_string(x0, y0 + 44, "szesz@webszos", COL_ACCENT, bg0);
    draw_string(x0 + 104, y0 + 44, "~/dev", COL_MAUVE, bg0);
    draw_string(x0 + 152, y0 + 44, ">", COL_GREEN, bg0);
    draw_string(x0 + 168, y0 + 44, term_input, COL_TEXT, bg0);
    if (focused_tile == 0) {
        draw_rect(x0 + 168 + (term_len * 8), y0 + 43, 8, 10, COL_TEXT);
    }

    draw_string(x0, y0 + 64, term_output, COL_YELLOW, bg0);

    // Tile 1: Editor Modal
    draw_editor_content();

    // Tile 2: Scripts
    u32 bg2 = (focused_tile == 2) ? COL_TILE_ACT : COL_SURFACE;
    u32 x2 = tiles[2].x + 12;
    u32 y2 = tiles[2].y + 30;
    u32 badge_x = tiles[2].x + tiles[2].w - 48;

    draw_string(x2, y2,      "build.sh",   COL_TEXT,   bg2);
    draw_string(badge_x, y2, "[F1]",       COL_YELLOW, bg2);

    draw_string(x2, y2 + 18, "meminfo.sh", COL_TEXT,   bg2);
    draw_string(badge_x, y2 + 18, "[F2]",  COL_YELLOW, bg2);

    draw_string(x2, y2 + 36, "export.sh",  COL_TEXT,   bg2);
    draw_string(badge_x, y2 + 36, "[F3]",  COL_YELLOW, bg2);

    draw_rect(x2, y2 + 56, tiles[2].w - 24, 1, COL_BORDER_IN);
    draw_string(x2, y2 + 66, script_status, COL_GREEN, bg2);
}

static void draw_bottombar(void) {
    u32 by = SCREEN_H - BAR_H;
    draw_rect(0, by, SCREEN_W, BAR_H, COL_BAR);
    draw_rect(0, by, SCREEN_W, 1, COL_BORDER_IN);

    if (focused_tile == 1 && editor_mode == MODE_INSERT) {
        draw_rect(6, by + 3, 64, 18, COL_ACCENT);
        draw_string(14, by + 8, "INSERT", COL_BG, COL_ACCENT);
        draw_string(82, by + 8, "ESC: Normal  |  Ctrl+S: Save to Browser", COL_TEXT, COL_BAR);
    } else {
        draw_rect(6, by + 3, 64, 18, COL_GREEN);
        draw_string(14, by + 8, "NORMAL", COL_BG, COL_GREEN);
        draw_string(82, by + 8, "Tab: Focus | i: Insert | h/l: Move | x: Del | F3: Save", COL_TEXT, COL_BAR);
    }

    draw_rect(SCREEN_W - 144, by + 3, 138, 18, COL_SURFACE);
    draw_string(SCREEN_W - 136, by + 8, "IPC: COM1 SYNC", COL_TEAL, COL_SURFACE);
}

void wm_render(void) {
    draw_rect(0, 0, SCREEN_W, SCREEN_H, COL_BG);
    draw_topbar();
    for (u32 i = 0; i < 3; i++) {
        draw_tile_frame(i);
    }
    draw_tiles_content();
    draw_bottombar();
}