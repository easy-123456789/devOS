#ifndef FAT32_H
#define FAT32_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct fat32_volume {
    uint32_t partition_lba;
    uint32_t total_sectors;
    uint32_t fat_lba;
    uint32_t fat_size_sectors;
    uint32_t first_data_sector;
    uint32_t sectors_per_cluster;
    uint32_t cluster_size_bytes;
    uint32_t root_cluster;
};

bool fat32_mount(struct fat32_volume *volume);
bool fat32_load_file(struct fat32_volume *volume, const char *path,
                     void *buffer, size_t capacity, size_t *file_size);

#endif
