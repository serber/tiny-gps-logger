#include "gps.h"

#include "main.h"

#define RX_DMA DMA1
#define RX_DMA_CHANNEL LL_DMA_CHANNEL_5
#define RX_DMA_REQUEST LL_DMA_REQUEST_2 /* USART1_RX, from the CubeMX DMA database. */

/* Output is 126 bytes per second, so this holds about 4 s across a slow SD write. */
#define RX_BUFFER_SIZE 512u

#define CONFIG_RETRY_MS 500u
#define NO_DATA_TIMEOUT_MS 5000u
#define RESET_PULSE_MS 10u
#define TX_BUFFER_SIZE 48u

typedef enum {
    GPS_CONFIG_SEND,
    GPS_CONFIG_WAIT,
    GPS_RUNNING,
} gps_state_t;

static uint8_t rx_buffer[RX_BUFFER_SIZE];
static uint32_t rx_tail;
static ubx_parser_t parser;

static gps_state_t state;
static uint32_t config_sent_ms;
static uint32_t last_frame_ms;
static bool alive;

static gps_fix_t pending_pvt;
static bool pending_pvt_valid;
static uint32_t pending_dop_itow;
static uint16_t pending_dop_hdop;
static bool pending_dop_valid;

static gps_fix_t ready_fix;
static bool ready_fix_valid;

static void uart_send(const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; i++) {
        while (!LL_USART_IsActiveFlag_TXE(USART1)) {
        }
        LL_USART_TransmitData8(USART1, data[i]);
    }
    while (!LL_USART_IsActiveFlag_TC(USART1)) {
    }
}

static void send_config(uint32_t now_ms)
{
    uint8_t frame[TX_BUFFER_SIZE];
    const size_t length = ubx_build_config(frame, sizeof frame);

    uart_send(frame, length);
    config_sent_ms = now_ms;
    state = GPS_CONFIG_WAIT;
}

static void pulse_reset(void)
{
    /* PB4 is open-drain: Set releases the line, it never drives it high. */
    LL_GPIO_ResetOutputPin(GPS_RESET_GPIO_Port, GPS_RESET_Pin);
    LL_mDelay(RESET_PULSE_MS);
    LL_GPIO_SetOutputPin(GPS_RESET_GPIO_Port, GPS_RESET_Pin);
}

static void try_publish_fix(void)
{
    if (pending_pvt_valid && pending_dop_valid && pending_pvt.itow_ms == pending_dop_itow) {
        ready_fix = pending_pvt;
        ready_fix.hdop_e2 = pending_dop_hdop;
        ready_fix_valid = true;
        pending_pvt_valid = false;
        pending_dop_valid = false;
    }
}

static void handle_frame(uint32_t now_ms)
{
    last_frame_ms = now_ms;
    alive = true;

    if (parser.msg_class == UBX_CLASS_NAV && parser.msg_id == UBX_NAV_PVT) {
        pending_pvt_valid = ubx_decode_pvt(parser.payload, parser.length, &pending_pvt);
        try_publish_fix();
    } else if (parser.msg_class == UBX_CLASS_NAV && parser.msg_id == UBX_NAV_DOP) {
        pending_dop_valid = ubx_decode_dop(parser.payload, parser.length, &pending_dop_itow,
                                           &pending_dop_hdop);
        try_publish_fix();
    } else if (parser.msg_class == UBX_CLASS_ACK && parser.length == 2 &&
               parser.payload[0] == UBX_CLASS_CFG && parser.payload[1] == UBX_CFG_VALSET &&
               state == GPS_CONFIG_WAIT) {
        state = (parser.msg_id == UBX_ACK_ACK) ? GPS_RUNNING : GPS_CONFIG_SEND;
    }
}

static void drain_rx(uint32_t now_ms)
{
    const uint32_t head = RX_BUFFER_SIZE - LL_DMA_GetDataLength(RX_DMA, RX_DMA_CHANNEL);

    while (rx_tail != (head % RX_BUFFER_SIZE)) {
        const uint8_t byte = rx_buffer[rx_tail];

        rx_tail = (rx_tail + 1u) % RX_BUFFER_SIZE;
        if (ubx_parser_feed(&parser, byte)) {
            handle_frame(now_ms);
        }
    }
}

void gps_init(uint32_t now_ms)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);

    LL_DMA_DisableChannel(RX_DMA, RX_DMA_CHANNEL);
    LL_DMA_SetPeriphRequest(RX_DMA, RX_DMA_CHANNEL, RX_DMA_REQUEST);
    LL_DMA_ConfigTransfer(RX_DMA, RX_DMA_CHANNEL,
                          LL_DMA_DIRECTION_PERIPH_TO_MEMORY | LL_DMA_MODE_CIRCULAR |
                              LL_DMA_PERIPH_NOINCREMENT | LL_DMA_MEMORY_INCREMENT |
                              LL_DMA_PDATAALIGN_BYTE | LL_DMA_MDATAALIGN_BYTE |
                              LL_DMA_PRIORITY_HIGH);
    LL_DMA_ConfigAddresses(RX_DMA, RX_DMA_CHANNEL,
                           LL_USART_DMA_GetRegAddr(USART1, LL_USART_DMA_REG_DATA_RECEIVE),
                           (uint32_t)rx_buffer, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetDataLength(RX_DMA, RX_DMA_CHANNEL, RX_BUFFER_SIZE);

    /* These bits can be changed only with the USART disabled. */
    LL_USART_Disable(USART1);
    LL_USART_DisableOverrunDetect(USART1);
    LL_USART_EnableDMAReq_RX(USART1);
    LL_DMA_EnableChannel(RX_DMA, RX_DMA_CHANNEL);
    LL_USART_Enable(USART1);

    rx_tail = 0;
    ubx_parser_reset(&parser);
    state = GPS_CONFIG_SEND;
    last_frame_ms = now_ms;
    alive = false;
}

void gps_poll(uint32_t now_ms)
{
    drain_rx(now_ms);

    switch (state) {
    case GPS_CONFIG_SEND:
        send_config(now_ms);
        break;
    case GPS_CONFIG_WAIT:
        if (now_ms - config_sent_ms >= CONFIG_RETRY_MS) {
            state = GPS_CONFIG_SEND;
        }
        break;
    case GPS_RUNNING:
    default:
        break;
    }

    if (now_ms - last_frame_ms >= NO_DATA_TIMEOUT_MS) {
        alive = false;
        last_frame_ms = now_ms;
        pulse_reset();
        state = GPS_CONFIG_SEND;
    }
}

bool gps_fix_pop(gps_fix_t *fix)
{
    if (!ready_fix_valid) {
        return false;
    }
    *fix = ready_fix;
    ready_fix_valid = false;
    return true;
}

bool gps_alive(void)
{
    return alive;
}

void gps_enter_backup(void)
{
    uint8_t frame[TX_BUFFER_SIZE];
    const size_t length = ubx_build_backup_request(frame, sizeof frame);

    uart_send(frame, length);
}
