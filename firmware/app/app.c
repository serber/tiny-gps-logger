#include "app.h"

#include "board.h"
#include "gps.h"
#include "logger.h"
#include "main.h"
#include "policy.h"
#include "power.h"

/* Bench option, enabled in Debug builds: log with USB present. */
#ifndef APP_LOG_ON_USB
#define APP_LOG_ON_USB 0
#endif

#define SYNC_PERIOD_MS 5000u
#define CARD_POLL_MS 100u
#define USB_POLL_MS 1000u
#define BATTERY_POLL_MS 10000u
#define FIX_FRESH_MS 3000u
#define GPS_SETTLE_MS 1000u /* Let the module boot before asking it to sleep. */

typedef enum {
    CARD_ABSENT,
    CARD_LOGGING,
    CARD_FAILED,
} card_state_t;

static card_state_t card_state = CARD_ABSENT;
static bool has_unsynced_rows;
static bool fix_seen;
static uint32_t last_fix_ms;
static uint32_t last_sync_ms;
static battery_filter_t battery_filter;

static void wait_with_wfi(uint32_t duration_ms)
{
    const uint32_t start = board_millis();

    while (board_millis() - start < duration_ms) {
        __WFI();
    }
}

/* USB present: the charger runs by itself, so stay out of its way until USB is removed. */
static void charge_loop(void)
{
    gps_init(board_millis());
    wait_with_wfi(GPS_SETTLE_MS);
    gps_enter_backup();
    board_set_led(false);

    while (power_usb_present()) {
        wait_with_wfi(USB_POLL_MS);
    }
    NVIC_SystemReset();
}

static void power_off_low_battery(void)
{
    (void)logger_stop();
    gps_enter_backup();
    board_set_led(false);
    board_shutdown();
}

static void start_new_file(uint32_t now_ms)
{
    card_state = logger_start() ? CARD_LOGGING : CARD_FAILED;
    has_unsynced_rows = false;
    last_sync_ms = now_ms;
}

/* A failed write or sync gets one recovery attempt, into a new file. */
static void recover_card(uint32_t now_ms)
{
    logger_abort();
    start_new_file(now_ms);
}

static void poll_card(uint32_t now_ms)
{
    const bool inserted = board_card_inserted();

    if (!inserted) {
        if (card_state == CARD_LOGGING) {
            logger_abort();
        }
        card_state = CARD_ABSENT;
    } else if (card_state == CARD_ABSENT) {
        start_new_file(now_ms);
    }
}

static void handle_fix(const gps_fix_t *fix, uint32_t now_ms)
{
    if (!gps_fix_is_valid(fix)) {
        return;
    }
    fix_seen = true;
    last_fix_ms = now_ms;

    if (card_state == CARD_LOGGING) {
        if (logger_write(fix)) {
            has_unsynced_rows = true;
        } else {
            recover_card(now_ms);
        }
    }
}

static void poll_sync(uint32_t now_ms)
{
    if (now_ms - last_sync_ms < SYNC_PERIOD_MS) {
        return;
    }
    last_sync_ms = now_ms;

    if (card_state == CARD_LOGGING && has_unsynced_rows) {
        if (logger_sync()) {
            has_unsynced_rows = false;
        } else {
            recover_card(now_ms);
        }
    }
}

static led_pattern_t current_led_pattern(uint32_t now_ms)
{
    if (card_state != CARD_LOGGING) {
        return LED_SD_ERROR;
    }
    if (!gps_alive()) {
        /* Allow the module its boot time and the first watchdog period. */
        return now_ms < 5000u ? LED_SEARCHING : LED_GPS_ERROR;
    }
    if (fix_seen && now_ms - last_fix_ms < FIX_FRESH_MS) {
        return LED_LOGGING;
    }
    return LED_SEARCHING;
}

static void log_loop(void)
{
    uint32_t last_card_ms = 0;
    uint32_t last_usb_ms = 0;
    uint32_t last_battery_ms = 0;
    gps_fix_t fix;

    gps_init(board_millis());
    poll_card(board_millis());

    for (;;) {
        const uint32_t now_ms = board_millis();

        gps_poll(now_ms);
        if (gps_fix_pop(&fix)) {
            handle_fix(&fix, now_ms);
        }
        poll_sync(now_ms);

        if (now_ms - last_card_ms >= CARD_POLL_MS) {
            last_card_ms = now_ms;
            poll_card(now_ms);
        }

        if (now_ms - last_usb_ms >= USB_POLL_MS) {
            last_usb_ms = now_ms;
            if (!APP_LOG_ON_USB && power_usb_present()) {
                (void)logger_stop();
                charge_loop();
            }
        }

        if (now_ms - last_battery_ms >= BATTERY_POLL_MS) {
            last_battery_ms = now_ms;
            /* VBAT reads 0 with SW1 off, so it means nothing with USB present. */
            if (!power_usb_present() && battery_filter_update(&battery_filter, power_vbat_mv())) {
                power_off_low_battery();
            }
        }

        board_set_led(led_pattern_level(current_led_pattern(now_ms), now_ms));
        __WFI(); /* SysTick wakes the core every millisecond. */
    }
}

void app_run(void)
{
    board_init();
    power_init();

    switch (policy_start(power_usb_present(), board_card_inserted(), APP_LOG_ON_USB != 0)) {
    case START_BOOTLOADER:
        board_enter_bootloader();
        break;
    case START_CHARGE:
        charge_loop();
        break;
    case START_LOG:
    default:
        log_loop();
        break;
    }
}
