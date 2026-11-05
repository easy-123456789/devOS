#include "fat32.h"
#include "sd.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define FAT32_SECTOR_SIZE 512u
#define FAT32_ENTRY_SIZE 32u
#define FAT32_END_OF_CHAIN 0x0FFFFFF8u
#define FAT32_BAD_CLUSTER  0x0FFFFFF7u

static uint32_t fat32_cluster_value(const struct fat32_volume *volume, uint32_t cluster)
{
    uint32_t fat_offset = cluster * 4u;
    uint32_t fat_sector = volume->reserved_sectors + (fat_offset / volume->bytes_per_sector);
    uint8_t sector[FAT32_SECTOR_SIZE];
    uint32_t value;

    if (!sd_read_block(fat_sector, sector)) {
        return FAT32_BAD_CLUSTER;
    }

    value = *(const uint32_t *)(const void *)(sector + (fat_offset % volume->bytes_per_sector));
    return value;
}

static bool fat32_read_cluster(const struct fat32_volume *volume, uint32_t cluster, uint8_t *buffer)
{
    uint32_t first_sector;
    uint32_t sector_index;

    if (cluster < 2u) {
        return false;
    }

    first_sector = volume->first_data_sector + (cluster - 2u) * volume->sectors_per_cluster;
    for (sector_index = 0u; sector_index < volume->sectors_per_cluster; ++sector_index) {
        if (!sd_read_block(first_sector + sector_index, buffer + (sector_index * FAT32_SECTOR_SIZE))) {
            return false;
        }
    }
    return true;
}

static void fat32_uppercase(char *dst, size_t dst_size, const char *src)
{
    size_t i = 0u;

    while (src[i] != '\0' && i + 1u < dst_size) {
        char ch = src[i];
        if (ch >= 'a' && ch <= 'z') {
            ch = (char)(ch - 'a' + 'A');
        }
        dst[i] = ch;
        ++i;
    }
    dst[i] = '\0';
}

static bool fat32_name_matches(const uint8_t *entry, const char *name)
{
    char short_name[13];
    unsigned i = 0u;
    unsigned j = 0u;

    while (i < 8u && entry[i] != ' ') {
        short_name[j++] = (char)entry[i++];
    }
    if (entry[8u] != ' ') {
        short_name[j++] = '.';
        i = 8u;
        while (i < 11u && entry[i] != ' ') {
            short_name[j++] = (char)entry[i++];
        }
    }
    short_name[j] = '\0';
    fat32_uppercase(short_name, sizeof(short_name), short_name);
    return strcmp(short_name, name) == 0;
}

bool fat32_init(struct fat32_volume *volume, const uint8_t *boot_sector)
{
    uint16_t bytes_per_sector;
    uint16_t reserved_sectors;
    uint8_t sectors_per_cluster;
    uint8_t num_fats;
    uint32_t total_sectors;
    uint32_t fat_size_sectors;
    uint32_t root_cluster;

    if (volume == NULL || boot_sector == NULL) {
        return false;
    }

    bytes_per_sector = (uint16_t)((uint16_t)boot_sector[11] | ((uint16_t)boot_sector[12] << 8u));
    sectors_per_cluster = boot_sector[13];
    reserved_sectors = (uint16_t)((uint16_t)boot_sector[14] | ((uint16_t)boot_sector[15] << 8u));
    num_fats = boot_sector[16];
    total_sectors = (uint32_t)((uint32_t)boot_sector[19] | ((uint32_t)boot_sector[20] << 8u) |
                               ((uint32_t)boot_sector[21] << 16u) | ((uint32_t)boot_sector[22] << 24u));
    fat_size_sectors = *(const uint32_t *)(const void *)(boot_sector + 0x24u);
    root_cluster = *(const uint32_t *)(const void *)(boot_sector + 0x2Cu);

    if (bytes_per_sector == 0u || sectors_per_cluster == 0u) {
        return false;
    }

    volume->bytes_per_sector = (uint32_t)bytes_per_sector;
    volume->sectors_per_cluster = (uint32_t)sectors_per_cluster;
    volume->reserved_sectors = (uint32_t)reserved_sectors;
    volume->fat_count = (uint32_t)num_fats;
    volume->fat_size_sectors = fat_size_sectors;
    volume->root_cluster = root_cluster;
    volume->first_data_sector = volume->reserved_sectors + (volume->fat_count * volume->fat_size_sectors);
    volume->total_sectors = total_sectors;
    volume->cluster_size_bytes = volume->bytes_per_sector * volume->sectors_per_cluster;
    return true;
}

bool fat32_find_file(struct fat32_volume *volume, const char *name, struct fat32_file *file)
{
    uint32_t cluster = volume->root_cluster;
    uint32_t current = 0u;
    uint8_t cluster_data[4096];

    if (volume == NULL || name == NULL || file == NULL) {
        return false;
    }

    while (cluster >= 2u && cluster < FAT32_END_OF_CHAIN) {
        uint32_t entry_index;
        if (!fat32_read_cluster(volume, cluster, cluster_data)) {
            return false;
        }

        for (entry_index = 0u; entry_index < volume->cluster_size_bytes; entry_index += FAT32_ENTRY_SIZE) {
            uint8_t *entry = cluster_data + entry_index;
            char normalized[13];
            uint8_t attr = entry[11];
            uint32_t first_cluster;
            uint32_t size_bytes;

            if (entry[0] == 0x00u) {
                return false;
            }
            if (entry[0] == 0xE5u) {
                continue;
            }
            if ((attr & 0x0Fu) == 0x0Fu) {
                continue;
            }
            if (attr & 0x08u) {
                continue;
            }

            if (entry[11] == 0x0Fu) {
                continue;
            }

            fat32_uppercase(normalized, sizeof(normalized), (const char *)entry);
            if (memcmp(normalized, name, strlen(name) + 1u) == 0) {
                first_cluster = (uint32_t)entry[26] | ((uint32_t)entry[27] << 8u);
                size_bytes = (uint32_t)entry[28] | ((uint32_t)entry[29] << 8u) |
                             ((uint32_t)entry[30] << 16u) | ((uint32_t)entry[31] << 24u);
                file->first_cluster = first_cluster;
                file->size_bytes = size_bytes;
                file->current_cluster = first_cluster;
                file->current_offset = 0u;
                return true;
            }
        }

        current = fat32_cluster_value(volume, cluster);
        if (current >= FAT32_END_OF_CHAIN || current == FAT32_BAD_CLUSTER) {
            break;
        }
        cluster = current;
    }

    return false;
}

bool fat32_read_file(struct fat32_volume *volume, const struct fat32_file *file,
                     uint8_t *buffer, size_t buffer_size, size_t *bytes_read)
{
    uint32_t cluster = file->first_cluster;
    size_t total = 0u;
    uint8_t cluster_data[4096];
    uint32_t remaining = file->size_bytes;

    if (volume == NULL || file == NULL || buffer == NULL) {
        return false;
    }

    while (cluster >= 2u && cluster < FAT32_END_OF_CHAIN && remaining > 0u) {
        uint32_t to_copy;
        uint32_t cluster_index;

        if (!fat32_read_cluster(volume, cluster, cluster_data)) {
            return false;
        }

        to_copy = volume->cluster_size_bytes;
        if (to_copy > remaining) {
            to_copy = remaining;
        }
        if (to_copy > buffer_size - total) {
            to_copy = (uint32_t)(buffer_size - total);
        }

        for (cluster_index = 0u; cluster_index < to_copy; ++cluster_index) {
            buffer[total + cluster_index] = cluster_data[cluster_index];
        }

        total += (size_t)to_copy;
        remaining -= to_copy;

        if (remaining == 0u) {
            break;
        }

        cluster = fat32_cluster_value(volume, cluster);
        if (cluster == FAT32_BAD_CLUSTER || cluster >= FAT32_END_OF_CHAIN) {
            break;
        }
    }

    if (bytes_read != NULL) {
        *bytes_read = total;
    }
    return true;
}
