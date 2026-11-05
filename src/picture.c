#include "picture.h"
#include "framebuffer.h"
#include "sd.h"
#include "uart.h"

#include <stddef.h>
#include <stdint.h>

#define FRAME_BG 0x00101828u
#define FRAME_PANEL 0x003080D0u
#define FRAME_ACCENT 0x00E8F0F8u

static void draw_loading_screen(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    framebuffer_clear(FRAME_BG);
    framebuffer_rect(x, y, width, height, FRAME_PANEL);
    framebuffer_rect(x + 8u, y + 8u, width - 16u, height - 16u, 0x00000000u);
    framebuffer_rect(x + 24u, y + 24u, width - 48u, 32u, FRAME_ACCENT);
    framebuffer_rect(x + 24u, y + 80u, width - 48u, height - 104u, 0x00000000u);
}

static int png_signature_valid(const uint8_t *buf)
{
    return buf[0] == 0x89u && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' &&
           buf[4] == '\r' && buf[5] == '\n' && buf[6] == 0x1au && buf[7] == '\n';
}

int picture_load_default_png(void)
{
    uint32_t width = framebuffer_width();
    uint32_t height = framebuffer_height();
    uint32_t x = (width > 320u) ? (width - 320u) / 2u : 0u;
    uint32_t y = (height > 240u) ? (height - 240u) / 2u : 0u;

    draw_loading_screen(x, y, 320u, 240u);
    uart_puts("PNG viewer pipeline initialized.\n");
    return 1;
}

int picture_load_sd_png(const char *path, uint32_t x, uint32_t y)
{
    uint8_t sector[512];
    (void)x;
    (void)y;

    if (!sd_init()) {
        uart_puts("SD card setup failed.\n");
        return 0;
    }

    /* The SD card driver is a stubbing layer for the bare-metal project; this hook
       is where a full FAT32 filesystem + PNG parser would be added. */
    if (!sd_read_block(0u, sector)) {
        uart_puts("No SD block data available.\n");
        return 0;
    }

    if (!png_signature_valid(sector)) {
        uart_puts("No PNG header found in the boot sector.\n");
        uart_puts("To render a PNG from SD, add FAT32 file lookup and PNG decoding next.\n");
        return 0;
    }

    /* The current filesystem implementation is intentionally limited to the image
       command hook; a full FAT32+PNG decoder needs more work before it is safe to
       trust it on real hardware. */
    uart_puts("Found a PNG header on the SD card.\n");
    uart_puts("The runtime PNG loader is still being wired up for FAT32 file reads.\n");
    return picture_load_default_png();
}

void picture_list_sd_pngs(void)
{
    uart_puts("PNG listing from SD is not yet implemented.\n");
}
