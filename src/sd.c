#include "sd.h"
#include "mmio.h"

#include <stddef.h>
#include <stdint.h>

#define EMMC_BASE       0x3F300000u
#define EMMC_ARG2       (EMMC_BASE + 0x00u)
#define EMMC_BLKSIZECNT (EMMC_BASE + 0x04u)
#define EMMC_ARG1       (EMMC_BASE + 0x08u)
#define EMMC_CMDTM      (EMMC_BASE + 0x0Cu)
#define EMMC_RESP0      (EMMC_BASE + 0x10u)
#define EMMC_DATA       (EMMC_BASE + 0x20u)
#define EMMC_STATUS     (EMMC_BASE + 0x24u)
#define EMMC_CONTROL0   (EMMC_BASE + 0x28u)
#define EMMC_CONTROL1   (EMMC_BASE + 0x2Cu)
#define EMMC_INTERRUPT  (EMMC_BASE + 0x30u)
#define EMMC_IRPT_MASK  (EMMC_BASE + 0x34u)
#define EMMC_IRPT_EN    (EMMC_BASE + 0x38u)

#define GPIO_BASE       0x3F200000u
#define GPIO_FSEL4      (GPIO_BASE + 0x10u)
#define GPIO_PUP_PDN0   (GPIO_BASE + 0xE4u)

#define SD_TIMEOUT      1000000u
#define SD_SECTOR_SIZE  512u
#define SD_INT_CMD_DONE (1u << 0)
#define SD_INT_DATA_DONE (1u << 1)
#define SD_INT_READ_READY (1u << 5)
#define SD_INT_ERROR    (1u << 15)
#define SD_INT_ERR_MASK 0xFFFF8000u
#define SD_STATUS_CMD_INHIBIT (1u << 0)
#define SD_STATUS_DAT_INHIBIT (1u << 1)
#define SD_CTRL1_RESET_ALL (1u << 24)
#define SD_CTRL1_CLOCK_INT_EN (1u << 0)
#define SD_CTRL1_CLOCK_STABLE (1u << 1)
#define SD_CTRL1_CLOCK_CARD_EN (1u << 2)

static int card_ready;
static int high_capacity;
static uint32_t card_rca;

static int wait_mask(uintptr_t reg, uint32_t mask, uint32_t expected)
{
    uint32_t i;
    for (i = 0; i < SD_TIMEOUT; ++i) {
        if ((mmio_read32(reg) & mask) == expected) return 1;
    }
    return 0;
}

static uint32_t command_word(unsigned index, unsigned response, uint32_t extra)
{
    return ((uint32_t)index << 24) | ((uint32_t)response << 16) | extra;
}

static int send_command(unsigned index, unsigned response_type, uint32_t extra,
                        uint32_t argument, uint32_t *response)
{
    uint32_t interrupt;
    uint32_t i;
    uint32_t inhibit = SD_STATUS_CMD_INHIBIT;
    if (extra & (1u << 21)) inhibit |= SD_STATUS_DAT_INHIBIT;

    if (!wait_mask(EMMC_STATUS, inhibit, 0u)) return 0;
    mmio_write32(EMMC_INTERRUPT, 0xFFFFFFFFu);
    mmio_write32(EMMC_ARG1, argument);
    mmio_write32(EMMC_CMDTM, command_word(index, response_type, extra));

    for (i = 0; i < SD_TIMEOUT; ++i) {
        interrupt = mmio_read32(EMMC_INTERRUPT);
        if (interrupt & SD_INT_ERROR) {
            mmio_write32(EMMC_INTERRUPT, interrupt);
            return 0;
        }
        if (interrupt & SD_INT_CMD_DONE) break;
    }
    if (i == SD_TIMEOUT) return 0;

    if (response != NULL) *response = mmio_read32(EMMC_RESP0);
    mmio_write32(EMMC_INTERRUPT, SD_INT_CMD_DONE);
    return 1;
}

static int app_command(uint32_t argument, uint32_t *response)
{
    uint32_t rca_response;
    if (!send_command(55u, 2u, (1u << 20) | (1u << 19), card_rca, &rca_response)) return 0;
    return send_command(41u, 2u, 0u, argument, response);
}

static void configure_sd_pins(void)
{
    uint32_t value = mmio_read32(GPIO_FSEL4);
    unsigned pin;
    /* GPIO48-53 ALT3 connect the SD card interface. */
    for (pin = 48u; pin <= 53u; ++pin) {
        unsigned shift = (pin % 10u) * 3u;
        value &= ~(7u << shift);
        value |= (7u << shift) & (3u << shift);
    }
    mmio_write32(GPIO_FSEL4, value);

    /* Disable pulls for GPIO48-53. */
    value = mmio_read32(GPIO_PUP_PDN0);
    for (pin = 48u; pin <= 53u; ++pin) value &= ~(3u << ((pin % 16u) * 2u));
    mmio_write32(GPIO_PUP_PDN0, value);
}

bool sd_init(void)
{
    uint32_t control1;
    uint32_t response;
    uint32_t ocr = 0u;
    unsigned attempt;

    card_ready = 0;
    high_capacity = 0;
    card_rca = 0u;
    configure_sd_pins();

    mmio_write32(EMMC_CONTROL1, SD_CTRL1_RESET_ALL);
    if (!wait_mask(EMMC_CONTROL1, SD_CTRL1_RESET_ALL, 0u)) return false;

    /* Start at about 400 kHz using the BCM2837 default 100 MHz EMMC clock. */
    control1 = mmio_read32(EMMC_CONTROL1);
    control1 &= ~0xFFE0u;
    control1 |= SD_CTRL1_CLOCK_INT_EN | (125u << 8);
    mmio_write32(EMMC_CONTROL1, control1);
    if (!wait_mask(EMMC_CONTROL1, SD_CTRL1_CLOCK_STABLE, SD_CTRL1_CLOCK_STABLE)) return false;
    mmio_write32(EMMC_CONTROL1, control1 | SD_CTRL1_CLOCK_CARD_EN);
    mmio_write32(EMMC_IRPT_MASK, 0xFFFFFFFFu);
    mmio_write32(EMMC_IRPT_EN, 0u);
    mmio_write32(EMMC_INTERRUPT, 0xFFFFFFFFu);

    if (!send_command(0u, 0u, 0u, 0u, NULL)) return false;
    /* CMD8 is absent on older SDSC cards; continue if it times out. */
    (void)send_command(8u, 2u, (1u << 20) | (1u << 19), 0x1AAu, &response);

    for (attempt = 0; attempt < 1000u; ++attempt) {
        if (!app_command(0x40FF8000u, &ocr)) return false;
        if (ocr & 0x80000000u) break;
    }
    if (!(ocr & 0x80000000u)) return false;
    high_capacity = (ocr & 0x40000000u) != 0u;

    if (!send_command(2u, 1u, 0u, 0u, NULL)) return false;
    if (!send_command(3u, 2u, 0u, 0u, &response)) return false;
    card_rca = response & 0xFFFF0000u;
    if (card_rca == 0u) return false;
    if (!send_command(7u, 3u, 0u, card_rca, NULL)) return false;

    if (!high_capacity && !send_command(16u, 2u, (1u << 20) | (1u << 19), SD_SECTOR_SIZE, NULL)) return false;

    /* Raise the bus clock to a conservative 25 MHz (divisor 4). */
    control1 = mmio_read32(EMMC_CONTROL1);
    control1 &= ~0xFFE0u;
    control1 |= SD_CTRL1_CLOCK_INT_EN | (2u << 8);
    mmio_write32(EMMC_CONTROL1, control1);
    if (!wait_mask(EMMC_CONTROL1, SD_CTRL1_CLOCK_STABLE, SD_CTRL1_CLOCK_STABLE)) return false;
    mmio_write32(EMMC_CONTROL1, control1 | SD_CTRL1_CLOCK_CARD_EN);

    card_ready = 1;
    return true;
}

bool sd_read_block(uint32_t block, void *dst)
{
    uint32_t argument;
    uint32_t interrupt;
    uint32_t i;
    uint32_t *words = (uint32_t *)dst;

    if (!card_ready || dst == NULL) return false;
    if (high_capacity) {
        argument = block;
    } else {
        if (block > 0x007FFFFFu) return false;
        argument = block * SD_SECTOR_SIZE;
    }

    mmio_write32(EMMC_BLKSIZECNT, (1u << 16) | SD_SECTOR_SIZE);
    if (!send_command(17u, 2u, (1u << 21) | (1u << 20) | (1u << 19) | (1u << 4), argument, NULL)) return false;

    for (i = 0; i < SD_TIMEOUT; ++i) {
        interrupt = mmio_read32(EMMC_INTERRUPT);
        if (interrupt & SD_INT_ERROR) {
            mmio_write32(EMMC_INTERRUPT, interrupt);
            return false;
        }
        if (interrupt & SD_INT_READ_READY) break;
    }
    if (i == SD_TIMEOUT) return false;

    for (i = 0; i < SD_SECTOR_SIZE / sizeof(uint32_t); ++i) words[i] = mmio_read32(EMMC_DATA);
    mmio_write32(EMMC_INTERRUPT, SD_INT_READ_READY);

    if (!wait_mask(EMMC_INTERRUPT, SD_INT_DATA_DONE, SD_INT_DATA_DONE)) return false;
    mmio_write32(EMMC_INTERRUPT, SD_INT_DATA_DONE);
    return true;
}

bool sd_write_block(uint32_t block, const void *src)
{
    (void)block;
    (void)src;
    /* Write support is intentionally omitted: the storage driver is read-only. */
    return false;
}
