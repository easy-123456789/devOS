#include "framebuffer.h"

#define MAILBOX_READ  (*(volatile uint32_t *)0x3F00B880u)
#define MAILBOX_STATUS (*(volatile uint32_t *)0x3F00B898u)
#define MAILBOX_WRITE (*(volatile uint32_t *)0x3F00B8A0u)
#define MAILBOX_EMPTY 0x40000000u
#define MAILBOX_FULL  0x80000000u
#define MAILBOX_FB_CHANNEL 8u
#define FB_WIDTH 640u
#define FB_HEIGHT 480u
#define FB_DEPTH 32u

static volatile uint32_t mailbox[35] __attribute__((aligned(16)));
static volatile uint32_t *pixels;
static uint32_t fb_pitch;
static uint32_t fb_width;
static uint32_t fb_height;

static int mailbox_call(uint8_t channel)
{
    uint32_t address = ((uint32_t)(uintptr_t)mailbox & 0x3FFFFFFFu) | 0x40000000u;

    while (MAILBOX_STATUS & MAILBOX_FULL) {
        __asm__ volatile("nop");
    }
    MAILBOX_WRITE = address | (channel & 0xFu);

    for (;;) {
        while (MAILBOX_STATUS & MAILBOX_EMPTY) {
            __asm__ volatile("nop");
        }
        uint32_t response = MAILBOX_READ;
        if ((response & 0xFu) == channel) {
            return (response & ~0xFu) == address;
        }
    }
}

int framebuffer_init(void)
{
    for (unsigned i = 0; i < 35; ++i) mailbox[i] = 0;
    mailbox[0] = 35 * 4;
    mailbox[1] = 0;
    mailbox[2] = 0x00048003; mailbox[3] = 8; mailbox[4] = 8;
    mailbox[5] = FB_WIDTH; mailbox[6] = FB_HEIGHT;
    mailbox[7] = 0x00048004; mailbox[8] = 8; mailbox[9] = 8;
    mailbox[10] = FB_WIDTH; mailbox[11] = FB_HEIGHT;
    mailbox[12] = 0x00048005; mailbox[13] = 4; mailbox[14] = 4;
    mailbox[15] = FB_DEPTH;
    mailbox[16] = 0x00048006; mailbox[17] = 4; mailbox[18] = 4;
    mailbox[19] = 1;
    mailbox[20] = 0x00040001; mailbox[21] = 8; mailbox[22] = 8;
    mailbox[23] = 16; mailbox[24] = 0;
    mailbox[25] = 0x00040008; mailbox[26] = 4; mailbox[27] = 4;
    mailbox[28] = 0;
    mailbox[29] = 0;
    mailbox[30] = 0;
    mailbox[31] = 0;
    mailbox[32] = 0;
    mailbox[33] = 0;
    mailbox[34] = 0;

    if (!mailbox_call(MAILBOX_FB_CHANNEL) || mailbox[1] != 0x80000000u ||
        mailbox[22] != 0x80000008u || mailbox[23] == 0 ||
        mailbox[27] != 0x80000004u || mailbox[28] == 0) {
        return 0;
    }

    fb_width = mailbox[5];
    fb_height = mailbox[6];
    fb_pitch = mailbox[28];
    pixels = (volatile uint32_t *)(uintptr_t)(mailbox[23] & 0x3FFFFFFFu);
    return pixels != 0 && fb_pitch >= fb_width * 4u && fb_width >= FB_WIDTH && fb_height >= FB_HEIGHT;
}

void framebuffer_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color)
{
    if (!pixels) return;
    if (x >= fb_width || y >= fb_height) return;
    if (width > fb_width - x) width = fb_width - x;
    if (height > fb_height - y) height = fb_height - y;
    for (uint32_t row = 0; row < height; ++row) {
        volatile uint32_t *line = (volatile uint32_t *)((uintptr_t)pixels + (uintptr_t)(y + row) * fb_pitch);
        for (uint32_t col = 0; col < width; ++col) line[x + col] = color;
    }
}

void framebuffer_blit(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                      const uint32_t *source)
{
    if (!pixels || !source || x >= fb_width || y >= fb_height) return;
    if (width > fb_width - x) width = fb_width - x;
    if (height > fb_height - y) height = fb_height - y;

    for (uint32_t row = 0; row < height; ++row) {
        volatile uint32_t *destination = (volatile uint32_t *)
            ((uintptr_t)pixels + (uintptr_t)(y + row) * fb_pitch);
        const uint32_t *source_row = source + (uintptr_t)row * width;
        for (uint32_t col = 0; col < width; ++col) {
            destination[x + col] = source_row[col];
        }
    }
}

void framebuffer_clear(uint32_t color)
{
    framebuffer_rect(0, 0, fb_width, fb_height, color);
}

uint32_t framebuffer_width(void) { return fb_width; }
uint32_t framebuffer_height(void) { return fb_height; }

void framebuffer_glyph(uint32_t x, uint32_t y, char ch, uint32_t fg, uint32_t bg)
{
    extern uint8_t font_row(char ch, unsigned row);
    for (unsigned row = 0; row < 8; ++row) {
        uint8_t bits = font_row(ch, row);
        for (unsigned col = 0; col < 8; ++col) {
            framebuffer_rect(x + col, y + row, 1, 1,
                (bits & (1u << (7u - col))) ? fg : bg);
        }
    }
}
