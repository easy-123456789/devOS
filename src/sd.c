#include "sd.h"
#include "mmio.h"

#include <stddef.h>
#include <stdint.h>

/* BCM2837 EMMC/SDHCI controller and GPIO block (Pi 3B+). */
#define EMMC_BASE       0x3F300000u
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
#define GPIO_GPFSEL4    (GPIO_BASE + 0x10u)
#define GPIO_GPFSEL5    (GPIO_BASE + 0x14u)
#define GPIO_GPPUD      (GPIO_BASE + 0x94u)
#define GPIO_GPPUDCLK1  (GPIO_BASE + 0x9Cu)

#define SD_TIMEOUT          2000000u
#define SD_SECTOR_SIZE      512u
#define SD_INT_CMD_DONE     (1u << 0)
#define SD_INT_DATA_DONE    (1u << 1)
#define SD_INT_READ_READY   (1u << 5)
#define SD_INT_ERROR        (1u << 15)
#define SD_INT_ERROR_MASK   0xFFFF8000u
#define SD_STATUS_CMD_BUSY  (1u << 0)
#define SD_STATUS_DAT_BUSY  (1u << 1)
#define SD_CTRL1_RESET_ALL  (1u << 24)
#define SD_CTRL1_CLK_INT_EN (1u << 0)
#define SD_CTRL1_CLK_STABLE (1u << 1)
#define SD_CTRL1_CLK_CARD_EN (1u << 2)
#define SD_CTRL1_TIMEOUT_MASK (0xFu << 16)

#define CMD_RESP_NONE 0u
#define CMD_RESP_136  1u
#define CMD_RESP_48   2u
#define CMD_RESP_48_BUSY 3u
#define CMD_CRC_CHECK (1u << 19)
#define CMD_INDEX_CHECK (1u << 20)
#define CMD_DATA_PRESENT (1u << 21)
#define CMD_TRANSFER_BLOCK_COUNT (1u << 1)
#define CMD_TRANSFER_READ (1u << 4)

static int card_ready;
static int high_capacity;
static uint32_t card_rca;

static int wait_register(uintptr_t reg, uint32_t mask, uint32_t expected)
{
    uint32_t i;
    for (i = 0u; i < SD_TIMEOUT; ++i) {
        if ((mmio_read32(reg) & mask) == expected) return 1;
    }
    return 0;
}

static uint32_t command_word(unsigned index, unsigned response, uint32_t flags)
{
    return ((uint32_t)index << 24) | ((uint32_t)response << 16) | flags;
}

static int send_command(unsigned index, unsigned response, uint32_t flags,
                        uint32_t argument, uint32_t *response0)
{
    uint32_t interrupt;
    uint32_t inhibit = SD_STATUS_CMD_BUSY;
    uint32_t i;

    if (flags & CMD_DATA_PRESENT) inhibit |= SD_STATUS_DAT_BUSY;
    if (!wait_register(EMMC_STATUS, inhibit, 0u)) return 0;

    mmio_write32(EMMC_INTERRUPT, 0xFFFFFFFFu);
    mmio_write32(EMMC_ARG1, argument);
    mmio_write32(EMMC_CMDTM, command_word(index, response, flags));

    for (i = 0u; i < SD_TIMEOUT; ++i) {
        interrupt = mmio_read32(EMMC_INTERRUPT);
        if (interrupt & SD_INT_ERROR) {
            mmio_write32(EMMC_INTERRUPT, interrupt & SD_INT_ERROR_MASK);
            return 0;
        }
        if (interrupt & SD_INT_CMD_DONE) break;
    }
    if (i == SD_TIMEOUT) return 0;

    if (response0 != NULL) *response0 = mmio_read32(EMMC_RESP0);
    mmio_write32(EMMC_INTERRUPT, SD_INT_CMD_DONE);
    return 1;
}

static int app_command(uint32_t argument, uint32_t *response)
{
    uint32_t ignored;
    uint32_t flags = CMD_CRC_CHECK | CMD_INDEX_CHECK;
    if (!send_command(55u, CMD_RESP_48, flags, card_rca, &ignored)) return 0;
    return send_command(41u, CMD_RESP_48, 0u, argument, response);
}

static uint32_t encode_clock_divisor(uint32_t divisor)
{
    /* SDHCI 10-bit divisor split between CLOCK_DIVISOR_HI and _LO. */
    return ((divisor & 0xFFu) << 8u) | ((divisor & 0x300u) >> 2u);
}

static int set_clock(uint32_t divisor)
{
    uint32_t control = mmio_read32(EMMC_CONTROL1);
    control &= ~(0xFFu << 8u | 3u << 6u | SD_CTRL1_CLK_CARD_EN);
    control |= SD_CTRL1_CLK_INT_EN | encode_clock_divisor(divisor);
    mmio_write32(EMMC_CONTROL1, control);
    if (!wait_register(EMMC_CONTROL1, SD_CTRL1_CLK_STABLE, SD_CTRL1_CLK_STABLE)) return 0;
    mmio_write32(EMMC_CONTROL1, control | SD_CTRL1_CLK_CARD_EN);
    return 1;
}

static void configure_sd_pins(void)
{
    uint32_t fsel4 = mmio_read32(GPIO_GPFSEL4);
    uint32_t fsel5 = mmio_read32(GPIO_GPFSEL5);
    uint32_t pull_mask = 0u;
    unsigned pin;

    /* GPIO48-53 use ALT3 for the EMMC interface. */
    for (pin = 48u; pin <= 53u; ++pin) {
        uint32_t shift = (uint32_t)(pin % 10u) * 3u;
        if (pin <= 49u) {
            fsel4 = (fsel4 & ~(7u << shift)) | (7u << shift);
        } else {
            fsel5 = (fsel5 & ~(7u << shift)) | (7u << shift);
        }
        pull_mask |= 1u << (pin - 32u);
    }
    mmio_write32(GPIO_GPFSEL4, fsel4);
    mmio_write32(GPIO_GPFSEL5, fsel5);

    /* Pi 3 legacy GPIO pull sequence: pull up CMD and data lines. */
    mmio_write32(GPIO_GPPUD, 2u);
    for (volatile unsigned delay = 0u; delay < 150u; ++delay) __asm__ volatile("nop");
    mmio_write32(GPIO_GPPUDCLK1, pull_mask & ~(1u << (48u - 32u)));
    for (volatile unsigned delay = 0u; delay < 150u; ++delay) __asm__ volatile("nop");
    mmio_write32(GPIO_GPPUD, 0u);
    mmio_write32(GPIO_GPPUDCLK1, 0u);
}

bool sd_init(void)
{
    uint32_t control;
    uint32_t response;
    uint32_t ocr = 0u;
    unsigned attempt;
    int version2;

    card_ready = 0;
    high_capacity = 0;
    card_rca = 0u;
    configure_sd_pins();

    mmio_write32(EMMC_CONTROL1, SD_CTRL1_RESET_ALL);
    if (!wait_register(EMMC_CONTROL1, SD_CTRL1_RESET_ALL, 0u)) return false;

    control = mmio_read32(EMMC_CONTROL1);
    control &= ~(SD_CTRL1_TIMEOUT_MASK | (0xFFu << 8u) | (3u << 6u));
    control |= (0xEu << 16u) | SD_CTRL1_CLK_INT_EN;
    mmio_write32(EMMC_CONTROL1, control);
    if (!set_clock(250u)) return false; /* 100 MHz / 250 = 400 kHz. */

    mmio_write32(EMMC_IRPT_MASK, 0xFFFFFFFFu);
    mmio_write32(EMMC_IRPT_EN, 0u);
    mmio_write32(EMMC_INTERRUPT, 0xFFFFFFFFu);

    if (!send_command(0u, CMD_RESP_NONE, 0u, 0u, NULL)) return false;

    /* Version-2 cards answer CMD8 with 0x1AA; older SDSC cards do not. */
    version2 = send_command(8u, CMD_RESP_48, CMD_CRC_CHECK | CMD_INDEX_CHECK, 0x1AAu, &response);
    if (version2 && (response & 0xFFFu) != 0x1AAu) return false;

    for (attempt = 0u; attempt < 1000u; ++attempt) {
        uint32_t requested_ocr = 0x00FF8000u;
        if (version2) requested_ocr |= 1u << 30;
        if (!app_command(requested_ocr, &ocr)) return false;
        if (ocr & 0x80000000u) break;
    }
    if (!(ocr & 0x80000000u)) return false;
    high_capacity = (ocr & 0x40000000u) != 0u;

    if (!send_command(2u, CMD_RESP_136, CMD_CRC_CHECK, 0u, NULL)) return false;
    if (!send_command(3u, CMD_RESP_48, CMD_CRC_CHECK | CMD_INDEX_CHECK, 0u, &response)) return false;
    card_rca = response & 0xFFFF0000u;
    if (card_rca == 0u) return false;
    if (!send_command(7u, CMD_RESP_48_BUSY, CMD_CRC_CHECK | CMD_INDEX_CHECK, card_rca, NULL)) return false;
    if (!wait_register(EMMC_STATUS, SD_STATUS_DAT_BUSY, 0u)) return false;

    if (!high_capacity && !send_command(16u, CMD_RESP_48,
            CMD_CRC_CHECK | CMD_INDEX_CHECK, SD_SECTOR_SIZE, NULL)) return false;

    /* Use the conservative default-speed 25 MHz clock, divisor 4. */
    if (!set_clock(4u)) return false;
    card_ready = 1;
    return true;
}

bool sd_read_block(uint32_t block, void *dst)
{
    uint8_t *bytes = (uint8_t *)dst;
    uint32_t argument;
    uint32_t interrupt;
    uint32_t i;

    if (!card_ready || dst == NULL) return false;
    if (high_capacity) {
        argument = block;
    } else {
        if (block > 0x007FFFFFu) return false;
        argument = block * SD_SECTOR_SIZE;
    }

    mmio_write32(EMMC_INTERRUPT, 0xFFFFFFFFu);
    mmio_write32(EMMC_BLKSIZECNT, (1u << 16u) | SD_SECTOR_SIZE);
    if (!send_command(17u, CMD_RESP_48,
            CMD_CRC_CHECK | CMD_INDEX_CHECK | CMD_DATA_PRESENT |
            CMD_TRANSFER_BLOCK_COUNT | CMD_TRANSFER_READ, argument, NULL)) return false;

    for (i = 0u; i < SD_TIMEOUT; ++i) {
        interrupt = mmio_read32(EMMC_INTERRUPT);
        if (interrupt & SD_INT_ERROR) {
            mmio_write32(EMMC_INTERRUPT, interrupt & SD_INT_ERROR_MASK);
            return false;
        }
        if (interrupt & SD_INT_READ_READY) break;
    }
    if (i == SD_TIMEOUT) return false;

    for (i = 0u; i < SD_SECTOR_SIZE / sizeof(uint32_t); ++i) {
        uint32_t word = mmio_read32(EMMC_DATA);
        bytes[i * 4u] = (uint8_t)word;
        bytes[i * 4u + 1u] = (uint8_t)(word >> 8u);
        bytes[i * 4u + 2u] = (uint8_t)(word >> 16u);
        bytes[i * 4u + 3u] = (uint8_t)(word >> 24u);
    }
    mmio_write32(EMMC_INTERRUPT, SD_INT_READ_READY);

    for (i = 0u; i < SD_TIMEOUT; ++i) {
        interrupt = mmio_read32(EMMC_INTERRUPT);
        if (interrupt & SD_INT_ERROR) {
            mmio_write32(EMMC_INTERRUPT, interrupt & SD_INT_ERROR_MASK);
            return false;
        }
        if (interrupt & SD_INT_DATA_DONE) break;
    }
    if (i == SD_TIMEOUT) return false;
    mmio_write32(EMMC_INTERRUPT, SD_INT_DATA_DONE);
    return 1;
}

bool sd_write_block(uint32_t block, const void *src)
{
    (void)block;
    (void)src;
    return false; /* Read-only driver: do not modify card contents. */
}
