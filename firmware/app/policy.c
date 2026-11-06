#include "policy.h"

#define VREFINT_CAL_MV 3000u /* ST calibrates VREFINT with VDDA at 3.0 V. */
#define ADC_FULL_SCALE 4095u

#define LED_PULSE_MS 50u
#define LED_PULSE_PERIOD_MS 200u

start_action_t policy_start(bool usb_present, bool card_inserted, bool log_on_usb)
{
    if (usb_present && !card_inserted) {
        return START_BOOTLOADER;
    }
    if (usb_present && !log_on_usb) {
        return START_CHARGE;
    }
    return START_LOG;
}

uint32_t policy_pin_mv(uint32_t raw_pin, uint32_t raw_vrefint, uint32_t vrefint_cal)
{
    if (raw_vrefint == 0) {
        return 0;
    }
    const uint32_t vdda_mv = VREFINT_CAL_MV * vrefint_cal / raw_vrefint;
    return vdda_mv * raw_pin / ADC_FULL_SCALE;
}

uint32_t policy_vbat_mv(uint32_t pin_mv)
{
    return pin_mv * 2u;
}

bool policy_usb_present(uint32_t pin_mv)
{
    return pin_mv >= USB_PRESENT_PIN_MV;
}

bool battery_filter_update(battery_filter_t *filter, uint32_t vbat_mv)
{
    if (vbat_mv >= BATTERY_CUTOFF_MV) {
        filter->low_count = 0;
    } else if (filter->low_count < BATTERY_LOW_READINGS) {
        filter->low_count++;
    }
    return filter->low_count >= BATTERY_LOW_READINGS;
}

bool led_pattern_level(led_pattern_t pattern, uint32_t t_ms)
{
    uint32_t period_ms;
    uint32_t pulses;

    switch (pattern) {
    case LED_SEARCHING:
        period_ms = 1000u;
        pulses = 1u;
        break;
    case LED_LOGGING:
        period_ms = 5000u;
        pulses = 1u;
        break;
    case LED_SD_ERROR:
        period_ms = 1000u;
        pulses = 2u;
        break;
    case LED_GPS_ERROR:
        period_ms = 1000u;
        pulses = 4u;
        break;
    case LED_OFF:
    default:
        return false;
    }

    const uint32_t phase = t_ms % period_ms;
    return phase < pulses * LED_PULSE_PERIOD_MS && phase % LED_PULSE_PERIOD_MS < LED_PULSE_MS;
}
