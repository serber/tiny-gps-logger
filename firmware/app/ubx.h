/* UBX protocol: frame parser, message decoding and frame building.
 * No hardware dependencies, so it is unit-tested on the host. */
#ifndef UBX_H
#define UBX_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UBX_CLASS_NAV 0x01
#define UBX_CLASS_RXM 0x02
#define UBX_CLASS_ACK 0x05
#define UBX_CLASS_CFG 0x06

#define UBX_NAV_DOP 0x04
#define UBX_NAV_PVT 0x07
#define UBX_RXM_PMREQ 0x41
#define UBX_ACK_NAK 0x00
#define UBX_ACK_ACK 0x01
#define UBX_CFG_VALSET 0x8A

#define UBX_NAV_PVT_LEN 92
#define UBX_NAV_DOP_LEN 18

/* NAV-PVT is the largest message the firmware reads. */
#define UBX_MAX_PAYLOAD UBX_NAV_PVT_LEN
#define UBX_FRAME_OVERHEAD 8

typedef struct {
    uint8_t state;
    uint8_t msg_class;
    uint8_t msg_id;
    uint8_t ck_a;
    uint8_t ck_b;
    uint16_t length;
    uint16_t count;
    uint8_t payload[UBX_MAX_PAYLOAD];
} ubx_parser_t;

typedef struct {
    uint32_t itow_ms;
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t fix_type;
    uint8_t sats;
    bool date_time_valid;
    bool gnss_fix_ok;
    int32_t lat_e7;
    int32_t lon_e7;
    int32_t hmsl_mm;
    int32_t speed_mm_s;
    int32_t course_e5;
    uint16_t hdop_e2;
} gps_fix_t;

void ubx_parser_reset(ubx_parser_t *parser);

/* Feed one received byte. Returns true when a complete frame with a valid
 * checksum is in the parser. The frame stays there until the next call. */
bool ubx_parser_feed(ubx_parser_t *parser, uint8_t byte);

/* Decode a NAV-PVT payload into fix. Does not touch hdop_e2. */
bool ubx_decode_pvt(const uint8_t *payload, uint16_t length, gps_fix_t *fix);

/* Decode a NAV-DOP payload. */
bool ubx_decode_dop(const uint8_t *payload, uint16_t length, uint32_t *itow_ms, uint16_t *hdop_e2);

/* A fix is logged only when the receiver vouches for position, date and time. */
bool gps_fix_is_valid(const gps_fix_t *fix);

/* Frame builders. They return the frame length, or 0 if out is too small. */
size_t ubx_build_frame(uint8_t *out, size_t capacity, uint8_t msg_class, uint8_t msg_id,
                       const uint8_t *payload, uint16_t length);

/* CFG-VALSET to the RAM layer: UBX only on UART1, NAV-PVT and NAV-DOP at 1 Hz. */
size_t ubx_build_config(uint8_t *out, size_t capacity);

/* RXM-PMREQ: enter backup mode until activity on the UART RX line. */
size_t ubx_build_backup_request(uint8_t *out, size_t capacity);

#endif
