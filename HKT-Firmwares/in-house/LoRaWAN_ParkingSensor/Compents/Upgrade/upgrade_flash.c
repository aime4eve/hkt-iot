/**
 * @file    upgrade_flash.c
 * @brief   P25Q40 SPI NOR flash driver for the upgrade path - see
 *          upgrade_flash.h. Register-level command set is the standard SFDP
 *          one (same as the W25Q family the reference design used), so any
 *          512KB-compatible drop-in works.
 */
#include <string.h>

#include "stm32l4xx_ll_bus.h"
#include "stm32l4xx_ll_gpio.h"
#include "stm32l4xx_ll_spi.h"

#include "upgrade_common.h"
#include "upgrade_flash.h"

/*==============================================================================
 * P25Q40 command set (same values as USER/Drive/include/spi.h enum)
 *============================================================================*/
#define CMD_WREN        0x06U
#define CMD_RDSR        0x05U
#define CMD_READ        0x03U
#define CMD_PP          0x02U
#define CMD_SER         0x20U   /* 4KB sector erase */
#define CMD_PWD         0xB9U
#define CMD_RES         0xABU   /* release power-down / device id */
#define CMD_RDJID       0x9FU   /* JEDEC id */

#define SR_WIP          0x01U

/* Worst-case program/erase times are ~3ms/400ms; 1s covers both with margin */
#define XFER_TIMEOUT_MS 1000U

/* SPI byte polling bound - far beyond one byte time even at 2MHz/16MHz HSI */
#define SPI_POLL_MAX    100000U

/* P25Q40 geometry - named with a UPG_ prefix to avoid clashing with the HAL's
 * internal-flash FLASH_PAGE_SIZE macro. */
#define UPG_NOR_PAGE_SIZE  256U
#define UPG_NOR_SECTOR     4096U

#define CS_HIGH()       LL_GPIO_SetOutputPin(GPIOA, LL_GPIO_PIN_15)
#define CS_LOW()        LL_GPIO_ResetOutputPin(GPIOA, LL_GPIO_PIN_15)
#define WP_HIGH()       LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_4)   /* SR writable */
#define WP_LOW()        LL_GPIO_ResetOutputPin(GPIOB, LL_GPIO_PIN_4)

/*==============================================================================
 * Low-level SPI3
 *============================================================================*/
static void spi3_init(void)
{
    LL_SPI_InitTypeDef  spi_init;
    LL_GPIO_InitTypeDef gpio_init;

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOA);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOB);
    LL_AHB2_GRP1_EnableClock(LL_AHB2_GRP1_PERIPH_GPIOC);

    /* PA15: soft CS, push-pull output, idle high */
    memset(&gpio_init, 0, sizeof(gpio_init));
    gpio_init.Pin       = LL_GPIO_PIN_15;
    gpio_init.Mode      = LL_GPIO_MODE_OUTPUT;
    gpio_init.Speed     = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull      = LL_GPIO_PULL_NO;
    LL_GPIO_Init(GPIOA, &gpio_init);
    CS_HIGH();

    /* PB4: /WP, idle low (same convention as USER/Drive/spi.c) */
    gpio_init.Pin = LL_GPIO_PIN_4;
    LL_GPIO_Init(GPIOB, &gpio_init);
    WP_LOW();

    /* PC10/11/12: SCK/MISO/MOSI, AF6 */
    gpio_init.Pin        = LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
    gpio_init.Mode       = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Alternate  = LL_GPIO_AF_6;
    LL_GPIO_Init(GPIOC, &gpio_init);

    memset(&spi_init, 0, sizeof(spi_init));
    spi_init.TransferDirection = LL_SPI_FULL_DUPLEX;
    spi_init.Mode              = LL_SPI_MODE_MASTER;
    spi_init.DataWidth         = LL_SPI_DATAWIDTH_8BIT;
    spi_init.ClockPolarity     = LL_SPI_POLARITY_LOW;
    spi_init.ClockPhase        = LL_SPI_PHASE_1EDGE;
    spi_init.NSS               = LL_SPI_NSS_SOFT;
    spi_init.BaudRate          = LL_SPI_BAUDRATEPRESCALER_DIV8;
    spi_init.BitOrder          = LL_SPI_MSB_FIRST;
    spi_init.CRCCalculation    = LL_SPI_CRCCALCULATION_DISABLE;
    spi_init.CRCPoly           = 7;
    LL_SPI_Init(SPI3, &spi_init);
    LL_SPI_SetStandard(SPI3, LL_SPI_PROTOCOL_MOTOROLA);
    LL_SPI_DisableNSSPulseMgt(SPI3);
    LL_SPI_Enable(SPI3);
}

static uint8_t spi3_byte(uint8_t tx)
{
    uint32_t guard = SPI_POLL_MAX;

    while (!LL_SPI_IsActiveFlag_TXE(SPI3)) {
        if (--guard == 0U) {
            return 0xFFU;
        }
    }
    LL_SPI_TransmitData8(SPI3, tx);
    guard = SPI_POLL_MAX;
    while (!LL_SPI_IsActiveFlag_RXNE(SPI3)) {
        if (--guard == 0U) {
            return 0xFFU;
        }
    }
    return LL_SPI_ReceiveData8(SPI3);
}

/* One CS assertion: write wlen bytes, then read rlen bytes. */
static void spi3_xfer(const uint8_t *w, uint16_t wlen, uint8_t *r, uint16_t rlen)
{
    uint16_t i;

    CS_LOW();
    WP_HIGH();
    for (i = 0; i < wlen; i++) {
        (void)spi3_byte(w[i]);
    }
    for (i = 0; i < rlen; i++) {
        r[i] = spi3_byte(0x00U);
    }
    WP_LOW();
    CS_HIGH();
}

/*==============================================================================
 * Flash primitives
 *============================================================================*/
static uint8_t read_sr(void)
{
    uint8_t cmd = CMD_RDSR;
    uint8_t sr = 0;

    spi3_xfer(&cmd, 1, &sr, 1);
    return sr;
}

static int wait_idle(void)
{
    uint32_t start = upg_tick_ms();

    while (read_sr() & SR_WIP) {
        if ((upg_tick_ms() - start) > XFER_TIMEOUT_MS) {
            return -1;
        }
    }
    return 0;
}

static void cmd_write_enable(void)
{
    uint8_t cmd = CMD_WREN;

    spi3_xfer(&cmd, 1, NULL, 0);
}

static int addr_ok(uint32_t addr, uint32_t len)
{
    return ((addr < UPG_EX_SIZE) && (len <= UPG_EX_SIZE) &&
            (addr + len <= UPG_EX_SIZE));
}

/*==============================================================================
 * Public API
 *============================================================================*/
int upg_flash_wake(void)
{
    uint8_t w[4] = {CMD_RES, 0xFFU, 0xFFU, 0xFFU};
    uint8_t id = 0;

    spi3_xfer(w, 4, &id, 1);    /* RES takes 3 dummy clocks before the DID */
    return ((id == 0x00U) || (id == 0xFFU)) ? -1 : 0;
}

void upg_flash_powerdown(void)
{
    uint8_t cmd = CMD_PWD;

    spi3_xfer(&cmd, 1, NULL, 0);
}

int upg_flash_init(void)
{
    uint8_t w[1] = {CMD_RDJID};
    uint8_t id[3] = {0, 0, 0};
    uint8_t i;

    spi3_init();
    (void)upg_flash_wake();

    spi3_xfer(w, 1, id, 3);

    /* Relaxed presence check (lesson from the reference boot): accept any
     * answering chip, reject the all-0x00 / all-0xFF degenerate reads. */
    for (i = 0; i < 3; i++) {
        if ((id[i] != 0x00U) && (id[i] != 0xFFU)) {
            return 0;
        }
    }
    return -1;
}

int upg_flash_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t  head[4];
    uint32_t done = 0;
    uint16_t chunk;

    if ((buf == NULL) || !addr_ok(addr, len)) {
        return -1;
    }

    while (done < len) {
        chunk = (uint16_t)UPG_MIN(len - done, 16384U);

        head[0] = CMD_READ;
        head[1] = (uint8_t)((addr + done) >> 16);
        head[2] = (uint8_t)((addr + done) >> 8);
        head[3] = (uint8_t)(addr + done);
        spi3_xfer(head, 4, buf + done, chunk);

        done += chunk;
    }
    return 0;
}

int upg_flash_write(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    /* static: single-threaded bare-metal flow, keeps 260B off the stack */
    static uint8_t frame[4 + UPG_NOR_PAGE_SIZE];
    uint32_t done = 0;
    uint32_t page_left;
    uint32_t chunk;

    if ((buf == NULL) || !addr_ok(addr, len)) {
        return -1;
    }

    while (done < len) {
        /* clip to the 256B page boundary: PP wraps within one page */
        page_left = UPG_NOR_PAGE_SIZE - ((addr + done) % UPG_NOR_PAGE_SIZE);
        chunk     = UPG_MIN(len - done, page_left);

        frame[0] = CMD_PP;
        frame[1] = (uint8_t)((addr + done) >> 16);
        frame[2] = (uint8_t)((addr + done) >> 8);
        frame[3] = (uint8_t)(addr + done);
        memcpy(frame + 4, buf + done, chunk);

        cmd_write_enable();
        spi3_xfer(frame, (uint16_t)(4 + chunk), NULL, 0);
        if (wait_idle() != 0) {
            return -1;
        }
        done += chunk;
    }
    return 0;
}

int upg_flash_erase_4k(uint32_t addr)
{
    uint8_t cmd[4];

    if (addr >= UPG_EX_SIZE) {
        return -1;
    }
    addr &= ~(uint32_t)(UPG_NOR_SECTOR - 1U);

    cmd[0] = CMD_SER;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)addr;

    cmd_write_enable();
    spi3_xfer(cmd, 4, NULL, 0);
    return wait_idle();
}
