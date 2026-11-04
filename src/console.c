#include "console.h"
#include "framebuffer.h"

#define BG 0x00101828u
#define FG 0x00E8F0F8u
#define ACCENT 0x003080D0u
#define CELL_W 8u
#define CELL_H 10u

static uint32_t cursor_x;
static uint32_t cursor_y;
static const uint32_t left = 16;
static const uint32_t top = 56;

void console_init(void)
{
    framebuffer_clear(BG);
    framebuffer_rect(0, 0, framebuffer_width(), 40, ACCENT);
    cursor_x = left;
    cursor_y = top;
}

void console_newline(void)
{
    cursor_x = left;
    cursor_y += CELL_H;
    if (cursor_y + CELL_H > framebuffer_height()) {
        framebuffer_clear(BG);
        framebuffer_rect(0, 0, framebuffer_width(), 40, ACCENT);
        cursor_y = top;
    }
}

void console_write(const char *text)
{
    while (*text) {
        char ch = *text++;
        if (ch == '\n') {
            console_newline();
            continue;
        }
        if (cursor_x + CELL_W >= framebuffer_width() - left) console_newline();
        framebuffer_glyph(cursor_x, cursor_y, ch, FG, BG);
        cursor_x += CELL_W;
    }
}
