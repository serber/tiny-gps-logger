/* Decisions with no hardware dependencies: what to do at start, battery state, LED patterns. */
#ifndef POLICY_H
#define POLICY_H

#include <stdbool.h>
#include <stdint.h>

#define BATTERY_CUTOFF_MV 3300u /* Working estimate, see firmware/SPEC.md. */
#define BATTERY_LOW_READINGS 3u
#define USB_PRESENT_PIN_MV 1500u /* About 2.5 V on the pin with USB connected. */

typedef enum {
    START_LOG,
    START_CHARGE,
    START_BOOTLOADER,
} start_action_t;

typedef enum {
    LED_OFF,
    LED_SEARCHING,
    LED_LOGGING,
    LED_SD_ERROR,
    LED_GPS_ERROR,
} led_pattern_t;

typedef struct {
    uint8_t low_count;
} battery_filter_t;

/* log_on_usb is the bench option: it allows logging with USB present. */
start_action_t policy_start(bool usb_present, bool card_inserted, bool log_on_usb);

/* Corrects for a falling supply using the factory VREFINT calibration value.
 * raw values are 12-bit ADC readings. */
uint32_t policy_pin_mv(uint32_t raw_pin, uint32_t raw_vrefint, uint32_t vrefint_cal);

/* The divider is 1:2. */
uint32_t policy_vbat_mv(uint32_t pin_mv);

bool policy_usb_present(uint32_t pin_mv);

/* Returns true after BATTERY_LOW_READINGS low readings in a row. */
bool battery_filter_update(battery_filter_t *filter, uint32_t vbat_mv);

/* LED level for a pattern at time t_ms. */
bool led_pattern_level(led_pattern_t pattern, uint32_t t_ms);

#endif
