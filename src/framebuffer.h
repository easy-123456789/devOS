#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdint.h>

int framebuffer_init(void);
void framebuffer_clear(uint32_t color);
void framebuffer_rect(uint32_t x, uint32_t y, uint32_t width, uint32_t height, uint32_t color);
void framebuffer_glyph(uint32_t x, uint32_t y, char ch, uint32_t fg, uint32_t bg);
uint32_t framebuffer_width(void);
uint32_t framebuffer_height(void);

#endif
