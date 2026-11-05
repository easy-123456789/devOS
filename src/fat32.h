#ifndef FAT32_H
#define FAT32_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct fat32_volume {
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_count;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;
    uint32_t first_data_sector;
    uint32_t total_sectors;
    uint32_t cluster_size_bytes;
};

struct fat32_file {
    uint32_t first_cluster;
    uint32_t size_bytes;
    uint32_t current_cluster;
    size_t current_offset;
};

bool fat32_init(struct fat32_volume *volume, const uint8_t *boot_sector);
bool fat32_find_file(struct fat32_volume *volume, const char *name, struct fat32_file *file);
bool fat32_read_file(struct fat32_volume *volume, const struct fat32_file *file,
                     uint8_t *buffer, size_t buffer_size, size_t *bytes_read);

#endif
