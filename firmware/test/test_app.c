/* Host tests for everything that has no hardware dependency. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csv.h"
#include "ff.h"
#include "logger.h"
#include "policy.h"
#include "ram_disk.h"
#include "ubx.h"
#include "ubx_vectors.h"

static int failures;
static int checks;

#define CHECK(cond)                                                     \
    do {                                                                \
        checks++;                                                       \
        if (!(cond)) {                                                  \
            failures++;                                                 \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
        }                                                               \
    } while (0)

#define CHECK_STR(actual, expected)                                     \
    do {                                                                \
        checks++;                                                       \
        if (strcmp((actual), (expected)) != 0) {                        \
            failures++;                                                 \
            printf("FAIL %s:%d:\n  got  %s\n  want %s\n", __FILE__,     \
                   __LINE__, (actual), (expected));                     \
        }                                                               \
    } while (0)

static bool feed(ubx_parser_t *parser, const uint8_t *data, size_t length, int *frames)
{
    bool last = false;

    for (size_t i = 0; i < length; i++) {
        last = ubx_parser_feed(parser, data[i]);
        if (last && frames != NULL) {
            (*frames)++;
        }
    }
    return last;
}

static gps_fix_t sample_fix(void)
{
    gps_fix_t fix;
    ubx_parser_t parser;

    ubx_parser_reset(&parser);
    (void)feed(&parser, FRAME_PVT, sizeof FRAME_PVT, NULL);
    memset(&fix, 0, sizeof fix);
    (void)ubx_decode_pvt(parser.payload, parser.length, &fix);
    fix.hdop_e2 = 82;
    return fix;
}

static void test_parser(void)
{
    ubx_parser_t parser;
    gps_fix_t fix;
    uint32_t itow = 0;
    uint16_t hdop = 0;
    int frames = 0;

    ubx_parser_reset(&parser);
    CHECK(feed(&parser, FRAME_PVT, sizeof FRAME_PVT, NULL));
    CHECK(parser.msg_class == UBX_CLASS_NAV && parser.msg_id == UBX_NAV_PVT);
    CHECK(ubx_decode_pvt(parser.payload, parser.length, &fix));
    CHECK(fix.itow_ms == 120015000u);
    CHECK(fix.year == 2026 && fix.month == 11 && fix.day == 6);
    CHECK(fix.hour == 9 && fix.minute == 30 && fix.second == 15);
    CHECK(fix.lat_e7 == 557558261 && fix.lon_e7 == 376173001);
    CHECK(fix.hmsl_mm == 152340 && fix.speed_mm_s == 1240 && fix.course_e5 == 8750000);
    CHECK(fix.sats == 14 && fix.fix_type == 3);
    CHECK(gps_fix_is_valid(&fix));

    CHECK(feed(&parser, FRAME_DOP, sizeof FRAME_DOP, NULL));
    CHECK(ubx_decode_dop(parser.payload, parser.length, &itow, &hdop));
    CHECK(itow == 120015000u && hdop == 82);

    /* NMEA text around frames, as before configuration takes effect. */
    static const char nmea[] = "$GNRMC,,V,,,,,,,,,,N*4D\r\n";
    (void)feed(&parser, (const uint8_t *)nmea, sizeof nmea - 1, &frames);
    (void)feed(&parser, FRAME_PVT, sizeof FRAME_PVT, &frames);
    (void)feed(&parser, (const uint8_t *)nmea, sizeof nmea - 1, &frames);
    (void)feed(&parser, FRAME_DOP, sizeof FRAME_DOP, &frames);
    CHECK(frames == 2);

    /* A repeated sync byte must not lose the frame. */
    frames = 0;
    (void)ubx_parser_feed(&parser, 0xB5);
    (void)feed(&parser, FRAME_DOP, sizeof FRAME_DOP, &frames);
    CHECK(frames == 1);

    /* Every single-bit error in the frame must be rejected or leave no frame. */
    for (size_t bit = 16; bit < sizeof FRAME_PVT * 8; bit++) {
        uint8_t bad[sizeof FRAME_PVT];

        memcpy(bad, FRAME_PVT, sizeof bad);
        bad[bit / 8] ^= (uint8_t)(1u << (bit % 8));
        frames = 0;
        ubx_parser_reset(&parser);
        (void)feed(&parser, bad, sizeof bad, &frames);
        if (frames != 0) {
            CHECK(!"corrupted frame accepted");
        }
    }

    /* A frame too long to store is skipped, and the next one is still read. */
    static const uint8_t too_long[] = {0xB5, 0x62, 0x0A, 0x04, 0xFF, 0x00};
    frames = 0;
    ubx_parser_reset(&parser);
    (void)feed(&parser, too_long, sizeof too_long, &frames);
    (void)feed(&parser, FRAME_DOP, sizeof FRAME_DOP, &frames);
    CHECK(frames == 1);

    /* Zero-length frame. */
    uint8_t poll[UBX_FRAME_OVERHEAD];
    size_t length = ubx_build_frame(poll, sizeof poll, 0x0A, 0x04, NULL, 0);
    CHECK(length == UBX_FRAME_OVERHEAD);
    CHECK(feed(&parser, poll, length, NULL));

    /* Our own builders parse back. */
    uint8_t out[64];
    length = ubx_build_config(out, sizeof out);
    CHECK(length != 0 && feed(&parser, out, length, NULL));
    CHECK(parser.msg_class == UBX_CLASS_CFG && parser.msg_id == UBX_CFG_VALSET);
    CHECK(ubx_build_config(out, 8) == 0);
    length = ubx_build_backup_request(out, sizeof out);
    CHECK(length == 24 && feed(&parser, out, length, NULL));
    CHECK(UBX_MAX_PAYLOAD >= UBX_NAV_PVT_LEN);

    CHECK(!ubx_decode_pvt(parser.payload, 91, &fix));
    CHECK(!ubx_decode_dop(parser.payload, 17, &itow, &hdop));
}

static void test_fix_validity(void)
{
    gps_fix_t fix = sample_fix();
    gps_fix_t bad;

    CHECK(gps_fix_is_valid(&fix));
    bad = fix;
    bad.fix_type = 2;
    CHECK(gps_fix_is_valid(&bad));
    for (uint8_t type = 0; type < 8; type++) {
        bad = fix;
        bad.fix_type = type;
        CHECK(gps_fix_is_valid(&bad) == (type == 2 || type == 3));
    }
    bad = fix;
    bad.gnss_fix_ok = false;
    CHECK(!gps_fix_is_valid(&bad));
    bad = fix;
    bad.date_time_valid = false;
    CHECK(!gps_fix_is_valid(&bad));
}

static void test_csv(void)
{
    char row[CSV_ROW_MAX];
    gps_fix_t fix = sample_fix();
    gps_fix_t south;
    ubx_parser_t parser;
    size_t length;

    length = csv_format_row(row, sizeof row, &fix);
    CHECK(length != 0);
    row[length] = '\0';
    CHECK_STR(row, "2026-11-06T09:30:15Z,55.7558261,37.6173001,152.3,1.24,87.5,14,0.82,3\r\n");

    /* Southern and western hemisphere, below sea level, zero speed. */
    ubx_parser_reset(&parser);
    CHECK(feed(&parser, FRAME_PVT_SOUTH, sizeof FRAME_PVT_SOUTH, NULL));
    memset(&south, 0, sizeof south);
    CHECK(ubx_decode_pvt(parser.payload, parser.length, &south));
    south.hdop_e2 = 1205;
    length = csv_format_row(row, sizeof row, &south);
    row[length] = '\0';
    CHECK_STR(row, "2026-01-02T03:04:05Z,-33.8688000,-0.1234567,-6.0,0.00,0.0,4,12.05,2\r\n");

    /* Rounding does not leave a negative zero. */
    south.hmsl_mm = -40;
    length = csv_format_row(row, sizeof row, &south);
    row[length] = '\0';
    CHECK(strstr(row, ",0.0,0.00,") != NULL);
    south.hmsl_mm = 49;
    length = csv_format_row(row, sizeof row, &south);
    row[length] = '\0';
    CHECK(strstr(row, ",0.0,0.00,") != NULL);
    south.hmsl_mm = 50;
    length = csv_format_row(row, sizeof row, &south);
    row[length] = '\0';
    CHECK(strstr(row, ",0.1,0.00,") != NULL);

    /* Longest possible row fits. */
    gps_fix_t wide;
    memset(&wide, 0, sizeof wide);
    wide.year = 65535;
    wide.month = wide.day = wide.hour = wide.minute = wide.second = 255;
    wide.lat_e7 = wide.lon_e7 = wide.hmsl_mm = INT32_MIN;
    wide.speed_mm_s = wide.course_e5 = INT32_MIN;
    wide.sats = wide.fix_type = 255;
    wide.hdop_e2 = 65535;
    CHECK(csv_format_row(row, sizeof row, &wide) != 0);

    /* Too small a buffer is reported and not overrun. */
    char small[16];
    memset(small, 'x', sizeof small);
    CHECK(csv_format_row(small, 10, &fix) == 0);
    CHECK(small[10] == 'x');

    uint32_t number = 0;
    char name[LOG_NAME_SIZE];

    CHECK(log_name_parse("LOG00042.CSV", &number) && number == 42);
    CHECK(log_name_parse("LOG99999.CSV", &number) && number == 99999);
    CHECK(!log_name_parse("LOG0004.CSV", &number));
    CHECK(!log_name_parse("LOG000042.CSV", &number));
    CHECK(!log_name_parse("LOG00042.TXT", &number));
    CHECK(!log_name_parse("LOG0004A.CSV", &number));
    CHECK(!log_name_parse("", &number));
    CHECK(!log_name_parse("FOO.BAR", &number));
    log_name_format(name, 7);
    CHECK_STR(name, "LOG00007.CSV");
    log_name_format(name, LOG_NUMBER_MAX);
    CHECK_STR(name, "LOG99999.CSV");
}

static void test_policy(void)
{
    CHECK(policy_start(false, true, false) == START_LOG);
    CHECK(policy_start(false, false, false) == START_LOG);
    CHECK(policy_start(true, true, false) == START_CHARGE);
    CHECK(policy_start(true, false, false) == START_BOOTLOADER);
    CHECK(policy_start(true, true, true) == START_LOG);
    CHECK(policy_start(true, false, true) == START_BOOTLOADER);

    /* VDDA 3.0 V: raw VREFINT equals the calibration value. */
    CHECK(policy_pin_mv(2048, 1655, 1655) == 1500);
    /* VDDA 2.7 V: VREFINT reads higher, and the same raw value means less. */
    const uint32_t raw_vref_27 = 1655u * 3000u / 2700u;
    const uint32_t mv = policy_pin_mv(2048, raw_vref_27, 1655);
    CHECK(mv >= 1345 && mv <= 1355);
    CHECK(policy_pin_mv(1000, 0, 1655) == 0);

    CHECK(policy_vbat_mv(1900) == 3800);
    CHECK(policy_usb_present(2500));
    CHECK(policy_usb_present(1500));
    CHECK(!policy_usb_present(1499));
    CHECK(!policy_usb_present(0));

    battery_filter_t filter = {0};

    CHECK(!battery_filter_update(&filter, 3299));
    CHECK(!battery_filter_update(&filter, 3299));
    CHECK(!battery_filter_update(&filter, 3300)); /* A good reading restarts the count. */
    CHECK(!battery_filter_update(&filter, 3299));
    CHECK(!battery_filter_update(&filter, 3299));
    CHECK(battery_filter_update(&filter, 3299));
    CHECK(battery_filter_update(&filter, 3299));
    CHECK(!battery_filter_update(&filter, 4000));
}

static unsigned rising_edges(led_pattern_t pattern, uint32_t period_ms)
{
    unsigned edges = 0;
    bool previous = false;

    for (uint32_t t = 0; t < period_ms; t++) {
        const bool level = led_pattern_level(pattern, t);

        edges += (level && !previous) ? 1u : 0u;
        previous = level;
    }
    return edges;
}

static void test_led(void)
{
    CHECK(rising_edges(LED_OFF, 10000) == 0);
    CHECK(rising_edges(LED_SEARCHING, 1000) == 1);
    CHECK(rising_edges(LED_LOGGING, 5000) == 1);
    CHECK(rising_edges(LED_SD_ERROR, 1000) == 2);
    CHECK(rising_edges(LED_GPS_ERROR, 1000) == 4);
    CHECK(led_pattern_level(LED_SEARCHING, 49));
    CHECK(!led_pattern_level(LED_SEARCHING, 50));
    CHECK(led_pattern_level(LED_SEARCHING, 1000));
}

static size_t read_file(const char *name, char *out, size_t capacity)
{
    FIL file;
    UINT count = 0;

    if (f_open(&file, name, FA_READ) != FR_OK) {
        return 0;
    }
    (void)f_read(&file, out, (UINT)capacity - 1, &count);
    (void)f_close(&file);
    out[count] = '\0';
    return count;
}

static unsigned count_lines(const char *text)
{
    unsigned lines = 0;

    for (; *text != '\0'; text++) {
        lines += (*text == '\n') ? 1u : 0u;
    }
    return lines;
}

static void format_volume(uint32_t sectors, BYTE type)
{
    static uint8_t work[4096];
    MKFS_PARM options = {type, 0, 0, 0, 0};

    ram_disk_create(sectors);
    CHECK(f_mkfs("", &options, work, sizeof work) == FR_OK);
}

static void create_file(const char *name)
{
    FIL file;

    CHECK(f_open(&file, name, FA_WRITE | FA_CREATE_NEW) == FR_OK);
    CHECK(f_close(&file) == FR_OK);
}

static void test_logger(void)
{
    static char text[64 * 1024];
    static FATFS check_fs;
    gps_fix_t fix = sample_fix();
    FILINFO info;

    /* 64 MB gives a real FAT32 volume. */
    format_volume(64u * 2048u, FM_FAT32);

    CHECK(logger_start());
    for (unsigned i = 0; i < 100; i++) {
        fix.second = (uint8_t)(i % 60);
        CHECK(logger_write(&fix));
    }
    CHECK(logger_stop());

    CHECK(logger_start());
    CHECK(logger_stop());

    CHECK(f_mount(&check_fs, "", 1) == FR_OK);
    CHECK(check_fs.fs_type == FS_FAT32);
    CHECK(read_file("LOG00001.CSV", text, sizeof text) != 0);
    CHECK(count_lines(text) == 101);
    CHECK(strncmp(text, CSV_HEADER, strlen(CSV_HEADER)) == 0);
    CHECK(read_file("LOG00002.CSV", text, sizeof text) == strlen(CSV_HEADER));

    /* The file time comes from the last fix. 2026-11-06 09:30:38, stored in 2 s steps. */
    CHECK(f_stat("LOG00001.CSV", &info) == FR_OK);
    CHECK(info.fdate == (((2026 - 1980) << 9) | (11 << 5) | 6));
    CHECK(info.ftime == ((9 << 11) | (30 << 5) | 19));

    /* The next number is above the highest, not the first free one. Other names are ignored. */
    create_file("LOG00099.CSV");
    create_file("NOTES.TXT");
    create_file("LOG1.CSV");
    (void)f_unmount("");
    CHECK(logger_start());
    CHECK(logger_stop());
    CHECK(f_mount(&check_fs, "", 1) == FR_OK);
    CHECK(f_stat("LOG00100.CSV", &info) == FR_OK);
    (void)f_unmount("");

    /* Power cut: rows up to the last sync survive, later ones are lost, the volume stays good. */
    CHECK(logger_start());
    for (unsigned i = 0; i < 5; i++) {
        CHECK(logger_write(&fix));
    }
    CHECK(logger_sync());
    for (unsigned i = 0; i < 3; i++) {
        CHECK(logger_write(&fix));
    }
    logger_abort();
    CHECK(f_mount(&check_fs, "", 1) == FR_OK);
    CHECK(read_file("LOG00101.CSV", text, sizeof text) != 0);
    CHECK(count_lines(text) == 6);
    (void)f_unmount("");

    /* Crossing many sectors with periodic syncs, as a long track does. */
    CHECK(logger_start());
    for (unsigned i = 0; i < 700; i++) {
        CHECK(logger_write(&fix));
        if (i % 5 == 4) {
            CHECK(logger_sync());
        }
    }
    CHECK(logger_stop());
    CHECK(f_mount(&check_fs, "", 1) == FR_OK);
    CHECK(read_file("LOG00102.CSV", text, sizeof text) != 0);
    CHECK(count_lines(text) == 701);
    (void)f_unmount("");

    /* All numbers used. */
    format_volume(64u * 2048u, FM_FAT32);
    CHECK(f_mount(&check_fs, "", 1) == FR_OK);
    create_file("LOG99999.CSV");
    (void)f_unmount("");
    CHECK(!logger_start());
    CHECK(!logger_write(&fix));
    CHECK(!logger_sync());

    /* A blank card without a file system. */
    ram_disk_create(2048);
    CHECK(!logger_start());

    /* A full volume is reported as a write failure. */
    format_volume(256, FM_FAT);
    CHECK(logger_start());
    bool write_failed = false;
    for (unsigned i = 0; i < 20000 && !write_failed; i++) {
        write_failed = !logger_write(&fix);
    }
    CHECK(write_failed);
    logger_abort();
}

int main(void)
{
    test_parser();
    test_fix_validity();
    test_csv();
    test_policy();
    test_led();
    test_logger();

    printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
