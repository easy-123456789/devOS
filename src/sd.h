// src/sd.h
#ifndef SD_H
#define SD_H

#include <stdint.h>
#include <stdbool.h>

bool sd_init(void);
bool sd_read_block(uint32_t block, void *dst);
bool sd_write_block(uint32_t block, const void *src);

#endif
