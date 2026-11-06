/* Battery and USB sensing through ADC1. */
#ifndef POWER_H
#define POWER_H

#include <stdbool.h>
#include <stdint.h>

void power_init(void);
bool power_usb_present(void);
uint32_t power_vbat_mv(void);

#endif
