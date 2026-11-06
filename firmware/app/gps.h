/* MAX-M10S on USART1: DMA reception, configuration and the fix stream. */
#ifndef GPS_H
#define GPS_H

#include <stdbool.h>
#include <stdint.h>

#include "ubx.h"

/* Start reception. The module is configured from gps_poll. */
void gps_init(uint32_t now_ms);

/* Process received bytes and run configuration and the no-data watchdog. */
void gps_poll(uint32_t now_ms);

/* Returns true once for each NAV-PVT and NAV-DOP pair of one epoch. */
bool gps_fix_pop(gps_fix_t *fix);

/* True while valid UBX frames keep arriving. */
bool gps_alive(void);

/* Put the module into backup mode. Traffic on its RX line wakes it again. */
void gps_enter_backup(void);

#endif
