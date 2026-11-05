#ifndef SD_H
#define SD_H

#include <stdbool.h>
#include <stdint.h>

#define SD_BLOCK_SIZE 512u

/* Initializes the BCM2837 EMMC/SD host. Read-only block access. */
bool sd_init(void);
bool sd_read_block(uint32_t block, void *dst);
bool sd_write_block(uint32_t block, const void *src);

#endif
