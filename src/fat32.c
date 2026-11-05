#include "fat32.h"
#include "sd.h"

#include <stddef.h>
#include <stdint.h>

#define FAT32_EOC 0x0FFFFFF8u
#define FAT32_BAD 0x0FFFFFF7u
#define FAT32_MAX_SECTORS_PER_CLUSTER 128u
#define FAT32_MAX_CLUSTER_BYTES 65536u

static uint16_t get_le16(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8u));
}

static uint32_t get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8u) |
           ((uint32_t)p[2] << 16u) | ((uint32_t)p[3] << 24u);
}

static int is_power_of_two(uint32_t n)
{
    return n != 0u && (n & (n - 1u)) == 0u;
}

static int parse_bpb(struct fat32_volume *v, const uint8_t *b, uint32_t base,
                     uint32_t partition_sectors)
{
    uint32_t total = get_le16(b + 19u);
    uint32_t fat_size = get_le16(b + 22u);
    uint32_t bytes_per_sector = get_le16(b + 11u);
    uint32_t sectors_per_cluster = b[13];
    uint32_t reserved = get_le16(b + 14u);
    uint32_t fats = b[16];
    uint32_t data_start;
    uint32_t cluster_count;

    if (total == 0u) total = get_le32(b + 32u);
    if (fat_size == 0u) fat_size = get_le32(b + 36u);
    if (bytes_per_sector != SD_BLOCK_SIZE || !is_power_of_two(sectors_per_cluster) ||
        sectors_per_cluster > FAT32_MAX_SECTORS_PER_CLUSTER || reserved == 0u ||
        fats == 0u || fat_size == 0u || total == 0u || get_le16(b + 17u) != 0u ||
        get_le16(b + 22u) != 0u || get_le32(b + 36u) == 0u || get_le16(b + 42u) != 0u ||
        get_le32(b + 44u) < 2u || b[510] != 0x55u || b[511] != 0xAAu) return 0;

    data_start = reserved + fats * fat_size;
    if (data_start >= total || total > partition_sectors || base > UINT32_MAX - total) return 0;
    cluster_count = (total - data_start) / sectors_per_cluster;
    if (cluster_count == 0u || get_le32(b + 44u) >= cluster_count + 2u) return 0;
    if (fat_size * SD_BLOCK_SIZE / 4u < cluster_count + 2u) return 0;

    v->partition_lba = base;
    v->total_sectors = total;
    v->fat_lba = base + reserved;
    v->fat_size_sectors = fat_size;
    v->first_data_sector = base + data_start;
    v->sectors_per_cluster = sectors_per_cluster;
    v->cluster_size_bytes = sectors_per_cluster * SD_BLOCK_SIZE;
    v->root_cluster = get_le32(b + 44u) & 0x0FFFFFFFu;
    return v->cluster_size_bytes <= FAT32_MAX_CLUSTER_BYTES;
}

bool fat32_mount(struct fat32_volume *volume)
{
    uint8_t sector[SD_BLOCK_SIZE];
    unsigned i;

    if (volume == NULL || !sd_init() || !sd_read_block(0u, sector)) return false;
    if (sector[510] != 0x55u || sector[511] != 0xAAu) return false;

    /* Accept a FAT32 superfloppy or locate a primary MBR FAT32 partition. */
    if (parse_bpb(volume, sector, 0u, UINT32_MAX)) return true;
    for (i = 0u; i < 4u; ++i) {
        const uint8_t *entry = sector + 446u + i * 16u;
        uint8_t type = entry[4];
        uint32_t start = get_le32(entry + 8u);
        uint32_t count = get_le32(entry + 12u);
        if ((type == 0x0Bu || type == 0x0Cu) && start != 0u && count != 0u &&
            sd_read_block(start, sector) && parse_bpb(volume, sector, start, count)) return true;
    }
    return false;
}

static uint32_t next_cluster(const struct fat32_volume *v, uint32_t cluster)
{
    uint8_t sector[SD_BLOCK_SIZE];
    uint32_t offset = cluster * 4u;
    uint32_t lba = v->fat_lba + offset / SD_BLOCK_SIZE;
    if (lba >= v->first_data_sector || !sd_read_block(lba, sector)) return FAT32_BAD;
    return get_le32(sector + offset % SD_BLOCK_SIZE) & 0x0FFFFFFFu;
}

static void make_short_name(const char *text, char name[11], int *valid)
{
    unsigned base = 0u, ext = 0u, i;
    int after_dot = 0;
    for (i = 0u; i < 11u; ++i) name[i] = ' ';
    *valid = 1;
    if (text == NULL || text[0] == '\0') { *valid = 0; return; }
    for (i = 0u; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c == '.') {
            if (after_dot || base == 0u) { *valid = 0; return; }
            after_dot = 1;
        } else {
            if (c == '/' || c == '\\' || c == ' ' || c == ':' || c == '*' || c == '?' ||
                c == '"' || c == '<' || c == '>' || c == '|') { *valid = 0; return; }
            if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
            if (after_dot) {
                if (++ext > 3u) { *valid = 0; return; }
                name[8u + ext - 1u] = c;
            } else {
                if (++base > 8u) { *valid = 0; return; }
                name[base - 1u] = c;
            }
        }
    }
    if (base == 0u) *valid = 0;
}

static int find_entry(const struct fat32_volume *v, uint32_t dir_cluster,
                      const char *name_text, uint8_t found[32])
{
    char wanted[11];
    int valid;
    uint32_t cluster = dir_cluster;
    uint32_t clusters_left = v->total_sectors / v->sectors_per_cluster;
    make_short_name(name_text, wanted, &valid);
    if (!valid) return 0;

    while (cluster >= 2u && cluster < FAT32_EOC && clusters_left-- != 0u) {
        uint32_t sector_index;
        for (sector_index = 0u; sector_index < v->sectors_per_cluster; ++sector_index) {
            uint8_t sector[SD_BLOCK_SIZE];
            uint32_t lba = v->first_data_sector + (cluster - 2u) * v->sectors_per_cluster + sector_index;
            unsigned slot;
            if (lba < v->first_data_sector || lba >= v->partition_lba + v->total_sectors ||
                !sd_read_block(lba, sector)) return 0;
            for (slot = 0u; slot < SD_BLOCK_SIZE; slot += 32u) {
                const uint8_t *entry = sector + slot;
                unsigned k;
                if (entry[0] == 0u) return 0;
                if (entry[0] == 0xE5u || entry[11] == 0x0Fu || (entry[11] & 0x08u)) continue;
                for (k = 0u; k < 11u && entry[k] == (uint8_t)wanted[k]; ++k) { }
                if (k == 11u) {
                    for (k = 0u; k < 32u; ++k) found[k] = entry[k];
                    return 1;
                }
            }
        }
        cluster = next_cluster(v, cluster);
        if (cluster == FAT32_BAD || cluster >= FAT32_EOC) break;
    }
    return 0;
}

bool fat32_load_file(struct fat32_volume *v, const char *path,
                     void *buffer, size_t capacity, size_t *file_size)
{
    uint8_t entry[32];
    uint8_t *out = (uint8_t *)buffer;
    uint32_t directory;
    size_t written = 0u;
    const char *component;

    if (v == NULL || path == NULL || buffer == NULL) return false;
    directory = v->root_cluster;
    component = path;

    for (;;) {
        char part[13];
        size_t n = 0u;
        int last;
        uint32_t first_cluster;
        while (*component == '/' || *component == '\\') ++component;
        while (component[n] != '\0' && component[n] != '/' && component[n] != '\\') {
            if (n >= sizeof(part) - 1u) return false;
            part[n] = component[n];
            ++n;
        }
        if (n == 0u) return false;
        part[n] = '\0';
        component += n;
        while (*component == '/' || *component == '\\') ++component;
        last = (*component == '\0');

        if (!find_entry(v, directory, part, entry)) return false;
        first_cluster = ((uint32_t)get_le16(entry + 20u) << 16u) | get_le16(entry + 26u);
        if (!last) {
            if (!(entry[11] & 0x10u) || first_cluster < 2u) return false;
            directory = first_cluster;
            continue;
        }
        if (entry[11] & 0x10u) return false;

        {
            uint32_t remaining = get_le32(entry + 28u);
            uint32_t cluster = first_cluster;
            uint32_t clusters_left = v->total_sectors / v->sectors_per_cluster;
            if ((size_t)remaining > capacity) return false;
            while (remaining != 0u) {
                uint32_t sector_index;
                if (cluster < 2u || cluster >= FAT32_EOC || clusters_left-- == 0u) return false;
                for (sector_index = 0u; sector_index < v->sectors_per_cluster && remaining != 0u; ++sector_index) {
                    uint8_t sector[SD_BLOCK_SIZE];
                    uint32_t lba = v->first_data_sector + (cluster - 2u) * v->sectors_per_cluster + sector_index;
                    uint32_t copy = remaining < SD_BLOCK_SIZE ? remaining : SD_BLOCK_SIZE;
                    uint32_t j;
                    if (lba >= v->partition_lba + v->total_sectors || !sd_read_block(lba, sector)) return false;
                    for (j = 0u; j < copy; ++j) out[written + j] = sector[j];
                    written += copy;
                    remaining -= copy;
                }
                if (remaining != 0u) cluster = next_cluster(v, cluster);
            }
        }
        if (file_size != NULL) *file_size = written;
        return true;
    }
}
