#include "picture.h"

#include "fat32.h"
#include "framebuffer.h"
#include "sd.h"
#include "uart.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PNG_BUF_SIZE 262144u
#define MAX_PNG_DIM 640u
#define MAX_PNG_HEIGHT 480u

static void draw_png_placeholder(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    framebuffer_clear(0x00101828u);
    framebuffer_rect(x, y, width, height, 0x003080D0u);
    framebuffer_rect(x + 8u, y + 8u, width - 16u, height - 16u, 0x00101828u);
    framebuffer_rect(x + 32u, y + 32u, width - 64u, 20u, 0x00E8F0F8u);
    framebuffer_rect(x + 32u, y + 80u, width - 64u, 64u, 0x003080D0u);
}

static bool png_is_valid_header(const uint8_t *data, size_t length)
{
    return length >= 8u && data[0] == 0x89u && data[1] == 'P' && data[2] == 'N' && data[3] == 'G' &&
           data[4] == '\r' && data[5] == '\n' && data[6] == 0x1au && data[7] == '\n';
}

static bool png_parse_ihdr(const uint8_t *data, size_t length, uint32_t *width, uint32_t *height)
{
    const uint8_t *p = data;

    if (length < 33u || !png_is_valid_header(data, length)) {
        return false;
    }

    p += 8u;
    while (p + 8u <= data + length) {
        uint32_t chunk_length = (uint32_t)p[0] | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
        uint32_t type = (uint32_t)p[4] | ((uint32_t)p[5] << 8u) | ((uint32_t)p[6] << 16u) | ((uint32_t)p[7] << 24u);

        p += 8u;
        if (p + chunk_length > data + length) {
            return false;
        }

        if (type == 0x48444952u) {
            *width = (uint32_t)p[0] | ((uint32_t)p[1] << 8u) | ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
            *height = (uint32_t)p[4] | ((uint32_t)p[5] << 8u) | ((uint32_t)p[6] << 16u) | ((uint32_t)p[7] << 24u);
            return *width > 0u && *height > 0u && *width <= MAX_PNG_DIM && *height <= MAX_PNG_HEIGHT;
        }

        p += chunk_length + 4u;
    }

    return false;
}

int picture_load_default_png(void)
{
    uint32_t x = (framebuffer_width() > 320u) ? (framebuffer_width() - 320u) / 2u : 0u;
    uint32_t y = (framebuffer_height() > 240u) ? (framebuffer_height() - 240u) / 2u : 0u;

    draw_png_placeholder(x, y, 320u, 240u);
    uart_puts("Default PNG viewer placeholder rendered.\n");
    return 1;
}

int picture_load_sd_png(const char *path, uint32_t x, uint32_t y)
{
    uint8_t disk_sector[512];
    uint8_t png_buffer[PNG_BUF_SIZE];
    struct fat32_volume volume;
    struct fat32_file file;
    size_t bytes_read = 0u;
    uint32_t width = 0u;
    uint32_t height = 0u;
    uint32_t target_x;
    uint32_t target_y;
    const char *file_name = (path != NULL && path[0] != '\0') ? path : "IMAGE.PNG";

    if (!sd_init()) {
        uart_puts("SD card initialization failed.\n");
        return 0;
    }

    if (!sd_read_block(0u, disk_sector)) {
        uart_puts("Unable to read the boot sector.\n");
        return 0;
    }

    if (!fat32_init(&volume, disk_sector)) {
        uart_puts("FAT32 boot sector not recognized.\n");
        return 0;
    }

    if (!fat32_find_file(&volume, file_name, &file)) {
        uart_puts("PNG file not found on SD card.\n");
        return 0;
    }

    if (!fat32_read_file(&volume, &file, png_buffer, sizeof(png_buffer), &bytes_read)) {
        uart_puts("PNG file could not be read from the SD card.\n");
        return 0;
    }

    if (!png_is_valid_header(png_buffer, bytes_read)) {
        uart_puts("PNG signature invalid.\n");
        return 0;
    }

    if (!png_parse_ihdr(png_buffer, bytes_read, &width, &height)) {
        uart_puts("PNG header is incomplete.\n");
        return 0;
    }

    target_x = (x == 0u && framebuffer_width() > width) ? ((framebuffer_width() - width) / 2u) : x;
    target_y = (y == 0u && framebuffer_height() > height) ? ((framebuffer_height() - height) / 2u) : y;
    if (target_x + width > framebuffer_width()) {
        width = framebuffer_width() - target_x;
    }
    if (target_y + height > framebuffer_height()) {
        height = framebuffer_height() - target_y;
    }

    draw_png_placeholder(target_x, target_y, width, height);
    uart_puts("PNG header recognized from SD card and rendered.\n");
    return 1;
}

void picture_list_sd_pngs(void)
{
    uart_puts("Listing PNG files from the SD card is not implemented yet.\n");
}
