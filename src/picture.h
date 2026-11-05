#ifndef PICTURE_H
#define PICTURE_H

#include <stdint.h>

int picture_load_sd_png(const char *path, uint32_t x, uint32_t y);
int picture_load_default_png(void);
void picture_list_sd_pngs(void);

#endif
