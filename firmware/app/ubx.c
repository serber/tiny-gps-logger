#include "ubx.h"

#include <string.h>

enum {
    ST_SYNC1,
    ST_SYNC2,
    ST_CLASS,
    ST_ID,
    ST_LEN1,
    ST_LEN2,
    ST_PAYLOAD,
    ST_CK_A,
    ST_CK_B,
};

/* Keys from the u-blox configuration database, confirmed against pyubx2. */
#define KEY_RATE_MEAS 0x30210001u         /* U2 */
#define KEY_UART1OUT_UBX 0x10740001u      /* L */
#define KEY_UART1OUT_NMEA 0x10740002u     /* L */
#define KEY_MSGOUT_NAV_PVT_UART1 0x20910007u /* U1 */
#define KEY_MSGOUT_NAV_DOP_UART1 0x20910039u /* U1 */

#define VALSET_LAYER_RAM 0x01u

#define PMREQ_FLAG_BACKUP 0x02u
#define PMREQ_WAKE_UART_RX 0x08u

#define PVT_VALID_DATE 0x01u
#define PVT_VALID_TIME 0x02u
#define PVT_FULLY_RESOLVED 0x04u
#define PVT_GNSS_FIX_OK 0x01u

static uint16_t get_u16(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t get_u32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static int32_t get_i32(const uint8_t *p)
{
    return (int32_t)get_u32(p);
}

static uint8_t *put_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
    return p + 4;
}

static void checksum_add(ubx_parser_t *parser, uint8_t byte)
{
    parser->ck_a = (uint8_t)(parser->ck_a + byte);
    parser->ck_b = (uint8_t)(parser->ck_b + parser->ck_a);
}

void ubx_parser_reset(ubx_parser_t *parser)
{
    parser->state = ST_SYNC1;
}

bool ubx_parser_feed(ubx_parser_t *parser, uint8_t byte)
{
    switch (parser->state) {
    case ST_SYNC1:
        if (byte == 0xB5) {
            parser->state = ST_SYNC2;
        }
        break;
    case ST_SYNC2:
        if (byte == 0x62) {
            parser->state = ST_CLASS;
            parser->ck_a = 0;
            parser->ck_b = 0;
        } else if (byte != 0xB5) {
            parser->state = ST_SYNC1;
        }
        break;
    case ST_CLASS:
        parser->msg_class = byte;
        checksum_add(parser, byte);
        parser->state = ST_ID;
        break;
    case ST_ID:
        parser->msg_id = byte;
        checksum_add(parser, byte);
        parser->state = ST_LEN1;
        break;
    case ST_LEN1:
        parser->length = byte;
        checksum_add(parser, byte);
        parser->state = ST_LEN2;
        break;
    case ST_LEN2:
        parser->length = (uint16_t)(parser->length | ((uint16_t)byte << 8));
        checksum_add(parser, byte);
        parser->count = 0;
        if (parser->length > UBX_MAX_PAYLOAD) {
            parser->state = ST_SYNC1; /* Not a message we read. */
        } else {
            parser->state = (parser->length == 0) ? ST_CK_A : ST_PAYLOAD;
        }
        break;
    case ST_PAYLOAD:
        parser->payload[parser->count++] = byte;
        checksum_add(parser, byte);
        if (parser->count == parser->length) {
            parser->state = ST_CK_A;
        }
        break;
    case ST_CK_A:
        parser->state = (byte == parser->ck_a) ? ST_CK_B : ST_SYNC1;
        break;
    case ST_CK_B:
        parser->state = ST_SYNC1;
        return byte == parser->ck_b;
    default:
        parser->state = ST_SYNC1;
        break;
    }
    return false;
}

bool ubx_decode_pvt(const uint8_t *payload, uint16_t length, gps_fix_t *fix)
{
    if (length != UBX_NAV_PVT_LEN) {
        return false;
    }

    const uint8_t valid = payload[11];
    const uint32_t need = PVT_VALID_DATE | PVT_VALID_TIME | PVT_FULLY_RESOLVED;

    fix->itow_ms = get_u32(&payload[0]);
    fix->year = get_u16(&payload[4]);
    fix->month = payload[6];
    fix->day = payload[7];
    fix->hour = payload[8];
    fix->minute = payload[9];
    fix->second = payload[10];
    fix->date_time_valid = (valid & need) == need;
    fix->fix_type = payload[20];
    fix->gnss_fix_ok = (payload[21] & PVT_GNSS_FIX_OK) != 0;
    fix->sats = payload[23];
    fix->lon_e7 = get_i32(&payload[24]);
    fix->lat_e7 = get_i32(&payload[28]);
    fix->hmsl_mm = get_i32(&payload[36]);
    fix->speed_mm_s = get_i32(&payload[60]);
    fix->course_e5 = get_i32(&payload[64]);
    return true;
}

bool ubx_decode_dop(const uint8_t *payload, uint16_t length, uint32_t *itow_ms, uint16_t *hdop_e2)
{
    if (length != UBX_NAV_DOP_LEN) {
        return false;
    }
    *itow_ms = get_u32(&payload[0]);
    *hdop_e2 = get_u16(&payload[12]);
    return true;
}

bool gps_fix_is_valid(const gps_fix_t *fix)
{
    return fix->gnss_fix_ok && fix->date_time_valid && (fix->fix_type == 2 || fix->fix_type == 3);
}

size_t ubx_build_frame(uint8_t *out, size_t capacity, uint8_t msg_class, uint8_t msg_id,
                       const uint8_t *payload, uint16_t length)
{
    const size_t total = (size_t)length + UBX_FRAME_OVERHEAD;
    uint8_t ck_a = 0;
    uint8_t ck_b = 0;

    if (total > capacity) {
        return 0;
    }

    out[0] = 0xB5;
    out[1] = 0x62;
    out[2] = msg_class;
    out[3] = msg_id;
    out[4] = (uint8_t)length;
    out[5] = (uint8_t)(length >> 8);
    if (length > 0) {
        memcpy(&out[6], payload, length);
    }
    for (size_t i = 2; i < total - 2; i++) {
        ck_a = (uint8_t)(ck_a + out[i]);
        ck_b = (uint8_t)(ck_b + ck_a);
    }
    out[total - 2] = ck_a;
    out[total - 1] = ck_b;
    return total;
}

static uint8_t *put_key_u8(uint8_t *p, uint32_t key, uint8_t value)
{
    p = put_u32(p, key);
    *p++ = value;
    return p;
}

size_t ubx_build_config(uint8_t *out, size_t capacity)
{
    uint8_t payload[32];
    uint8_t *p = payload;

    *p++ = 0x00; /* version */
    *p++ = VALSET_LAYER_RAM;
    *p++ = 0x00;
    *p++ = 0x00;

    p = put_u32(p, KEY_RATE_MEAS);
    *p++ = (uint8_t)(1000u & 0xFFu);
    *p++ = (uint8_t)(1000u >> 8);

    p = put_key_u8(p, KEY_UART1OUT_UBX, 1);
    p = put_key_u8(p, KEY_UART1OUT_NMEA, 0);
    p = put_key_u8(p, KEY_MSGOUT_NAV_PVT_UART1, 1);
    p = put_key_u8(p, KEY_MSGOUT_NAV_DOP_UART1, 1);

    return ubx_build_frame(out, capacity, UBX_CLASS_CFG, UBX_CFG_VALSET, payload,
                           (uint16_t)(p - payload));
}

size_t ubx_build_backup_request(uint8_t *out, size_t capacity)
{
    uint8_t payload[16] = {0};
    uint8_t *p = &payload[4]; /* version and reserved bytes stay 0 */

    p = put_u32(p, 0);                 /* duration: until woken */
    p = put_u32(p, PMREQ_FLAG_BACKUP); /* flags */
    (void)put_u32(p, PMREQ_WAKE_UART_RX);

    return ubx_build_frame(out, capacity, UBX_CLASS_RXM, UBX_RXM_PMREQ, payload, sizeof payload);
}
