typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

#define COM1 0x3F8

static inline void outb(u16 port, u8 v) {
    __asm__ volatile ("outb %0, %1" : : "a"(v), "Nd"(port));
}
static inline u8 inb(u16 port) {
    u8 v;
    __asm__ volatile ("inb %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}
static inline void outw(u16 port, u16 v) {
    __asm__ volatile ("outw %0, %1" : : "a"(v), "Nd"(port));
}
static inline void outl(u16 port, u32 v) {
    __asm__ volatile ("outl %0, %1" : : "a"(v), "Nd"(port));
}
static inline u32 inl(u16 port) {
    u32 v;
    __asm__ volatile ("inl %1, %0" : "=a"(v) : "Nd"(port));
    return v;
}

static void dbg(char c) { outb(COM1, (u8)c); }

static void serial_putc(char c) {
    for (u32 i = 0; i < 100000; i++) {
        if (inb(COM1 + 5) & 0x20) break;
    }
    outb(COM1, (u8)c);
}
static void serial_puts(const char *s) { while (*s) serial_putc(*s++); }

static void serial_puthex(u32 v) {
    const char *hex = "0123456789ABCDEF";
    serial_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        serial_putc(hex[(v >> i) & 0xF]);
    }
}

static char serial_getc(void) {
    while (!(inb(COM1 + 5) & 0x01)) {}
    return (char)inb(COM1);
}
static int streq(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *a == *b;
}

static void vbe_write(u16 index, u16 value) {
    outw(0x01CE, index);
    outw(0x01CF, value);
}

static void vbe_set_mode(u16 width, u16 height, u16 bpp) {
    vbe_write(4, 0x00);
    vbe_write(1, width);
    vbe_write(2, height);
    vbe_write(3, bpp);
    vbe_write(4, 0x41);
}

static u32 get_vbe_lfb_addr(void) {
    outl(0x0CF8, 0x80001010);
    return inl(0x0CFC) & 0xFFFFFFF0;
}

void kmain(void) {
    char buf[64];
    dbg('1');
    serial_puts("[webos] boot ok: kernel i386 em C\r\n");
    dbg('2');

    vbe_set_mode(800, 600, 32);

    u32 lfb = get_vbe_lfb_addr();
    serial_puts("LFB addr: ");
    serial_puthex(lfb);
    serial_puts("\r\n");

    u32 *fb = (u32 *)lfb;
    for (u32 y = 0; y < 600; y++) {
        for (u32 x = 0; x < 800; x++) {
            fb[y * 800 + x] = 0x00008080;
        }
    }

    for (;;) {
        serial_puts("webos> ");
        u32 n = 0;
        char c;
        while ((c = serial_getc()) != '\n' && c != '\r') {
            if (n < sizeof buf - 1) { buf[n++] = c; serial_putc(c); }
        }
        buf[n] = 0;
        serial_puts("\r\n");
        if      (streq(buf, "ping"))    serial_puts("pong (respondido pelo guest)\r\n");
        else if (streq(buf, "version")) serial_puts("WebOS Lab M0\r\n");
        else if (streq(buf, "help"))    serial_puts("ping version help\r\n");
        else if (n)                     serial_puts("comando desconhecido\r\n");
    }
}