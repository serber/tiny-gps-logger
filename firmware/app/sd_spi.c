#include "sd_spi.h"

#include "board.h"
#include "main.h"

#define CMD0 0u
#define CMD8 8u
#define CMD12 12u
#define CMD16 16u
#define CMD17 17u
#define CMD24 24u
#define CMD55 55u
#define CMD58 58u
#define ACMD41 (0x80u | 41u)

#define R1_IDLE 0x01u
#define TOKEN_DATA 0xFEu
#define DATA_RESPONSE_MASK 0x1Fu
#define DATA_ACCEPTED 0x05u
#define SECTOR_SIZE 512u

#define CMD0_TRIES 5u
#define INIT_TIMEOUT_MS 1000u
#define READY_TIMEOUT_MS 500u
#define READ_TOKEN_TIMEOUT_MS 200u

/* 16 MHz / 64 = 250 kHz during init, 16 MHz / 2 = 8 MHz afterwards. */
#define SPI_SLOW LL_SPI_BAUDRATEPRESCALER_DIV64
#define SPI_FAST LL_SPI_BAUDRATEPRESCALER_DIV2

static bool byte_addressed;

static uint8_t xfer(uint8_t out)
{
    while (!LL_SPI_IsActiveFlag_TXE(SPI1)) {
    }
    LL_SPI_TransmitData8(SPI1, out);
    while (!LL_SPI_IsActiveFlag_RXNE(SPI1)) {
    }
    return LL_SPI_ReceiveData8(SPI1);
}

static void set_clock(uint32_t prescaler)
{
    while (LL_SPI_IsActiveFlag_BSY(SPI1)) {
    }
    LL_SPI_Disable(SPI1);
    LL_SPI_SetBaudRatePrescaler(SPI1, prescaler);
    LL_SPI_SetRxFIFOThreshold(SPI1, LL_SPI_RX_FIFO_TH_QUARTER); /* RXNE on each byte */
    LL_SPI_Enable(SPI1);
}

static bool wait_ready(uint32_t timeout_ms)
{
    const uint32_t start = board_millis();

    do {
        if (xfer(0xFF) == 0xFF) {
            return true;
        }
    } while (board_millis() - start < timeout_ms);
    return false;
}

static void deselect(void)
{
    LL_GPIO_SetOutputPin(SD_CS_GPIO_Port, SD_CS_Pin);
    (void)xfer(0xFF); /* The card releases MISO on the next clock. */
}

static bool select_card(void)
{
    LL_GPIO_ResetOutputPin(SD_CS_GPIO_Port, SD_CS_Pin);
    (void)xfer(0xFF);
    if (wait_ready(READY_TIMEOUT_MS)) {
        return true;
    }
    deselect();
    return false;
}

static uint8_t send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t response;

    if ((cmd & 0x80u) != 0u) {
        cmd &= 0x7Fu;
        response = send_cmd(CMD55, 0);
        if (response > R1_IDLE) {
            return response;
        }
    }

    if (cmd != CMD12) {
        deselect();
        if (!select_card()) {
            return 0xFF;
        }
    }

    (void)xfer((uint8_t)(0x40u | cmd));
    (void)xfer((uint8_t)(arg >> 24));
    (void)xfer((uint8_t)(arg >> 16));
    (void)xfer((uint8_t)(arg >> 8));
    (void)xfer((uint8_t)arg);
    /* Only CMD0 and CMD8 are checked in SPI mode. */
    (void)xfer((cmd == CMD0) ? 0x95u : (cmd == CMD8) ? 0x87u : 0x01u);

    if (cmd == CMD12) {
        (void)xfer(0xFF);
    }

    unsigned tries = 10;
    do {
        response = xfer(0xFF);
    } while ((response & 0x80u) != 0u && --tries != 0u);
    return response;
}

static bool wait_acmd41(uint32_t arg)
{
    const uint32_t start = board_millis();

    do {
        if (send_cmd(ACMD41, arg) == 0u) {
            return true;
        }
    } while (board_millis() - start < INIT_TIMEOUT_MS);
    return false;
}

static bool init_v2(void)
{
    uint8_t reply[4];

    for (unsigned i = 0; i < sizeof reply; i++) {
        reply[i] = xfer(0xFF);
    }
    if (reply[2] != 0x01u || reply[3] != 0xAAu) {
        return false; /* The card does not accept 2.7-3.6 V. */
    }
    if (!wait_acmd41(1uL << 30) || send_cmd(CMD58, 0) != 0u) {
        return false;
    }
    for (unsigned i = 0; i < sizeof reply; i++) {
        reply[i] = xfer(0xFF);
    }
    byte_addressed = (reply[0] & 0x40u) == 0u; /* CCS bit of the OCR */
    return !byte_addressed || send_cmd(CMD16, SECTOR_SIZE) == 0u;
}

static bool init_v1(void)
{
    byte_addressed = true;
    return wait_acmd41(0) && send_cmd(CMD16, SECTOR_SIZE) == 0u;
}

bool sd_init(void)
{
    bool ok = false;

    set_clock(SPI_SLOW);
    LL_GPIO_SetOutputPin(SD_CS_GPIO_Port, SD_CS_Pin);
    for (unsigned i = 0; i < 10; i++) {
        (void)xfer(0xFF); /* At least 74 clocks with CS high. */
    }

    for (unsigned i = 0; i < CMD0_TRIES && !ok; i++) {
        if (send_cmd(CMD0, 0) == R1_IDLE) {
            ok = (send_cmd(CMD8, 0x1AAu) == R1_IDLE) ? init_v2() : init_v1();
            break;
        }
    }
    deselect();

    if (ok) {
        set_clock(SPI_FAST);
    }
    return ok;
}

static uint32_t card_address(uint32_t sector)
{
    return byte_addressed ? sector * SECTOR_SIZE : sector;
}

static bool read_block(uint8_t *buffer)
{
    const uint32_t start = board_millis();
    uint8_t token;

    do {
        token = xfer(0xFF);
    } while (token == 0xFFu && board_millis() - start < READ_TOKEN_TIMEOUT_MS);
    if (token != TOKEN_DATA) {
        return false;
    }

    for (unsigned i = 0; i < SECTOR_SIZE; i++) {
        buffer[i] = xfer(0xFF);
    }
    (void)xfer(0xFF); /* CRC */
    (void)xfer(0xFF);
    return true;
}

static bool write_block(const uint8_t *buffer)
{
    (void)xfer(TOKEN_DATA);
    for (unsigned i = 0; i < SECTOR_SIZE; i++) {
        (void)xfer(buffer[i]);
    }
    (void)xfer(0xFF); /* CRC */
    (void)xfer(0xFF);
    return (xfer(0xFF) & DATA_RESPONSE_MASK) == DATA_ACCEPTED;
}

bool sd_read(uint8_t *buffer, uint32_t sector, uint32_t count)
{
    bool ok = true;

    for (uint32_t i = 0; ok && i < count; i++) {
        ok = send_cmd(CMD17, card_address(sector + i)) == 0u && read_block(buffer);
        buffer += SECTOR_SIZE;
    }
    deselect();
    return ok;
}

bool sd_write(const uint8_t *buffer, uint32_t sector, uint32_t count)
{
    bool ok = true;

    for (uint32_t i = 0; ok && i < count; i++) {
        ok = send_cmd(CMD24, card_address(sector + i)) == 0u && write_block(buffer);
        buffer += SECTOR_SIZE;
    }
    deselect();
    return ok;
}

bool sd_sync(void)
{
    const bool ok = select_card();

    deselect();
    return ok;
}
