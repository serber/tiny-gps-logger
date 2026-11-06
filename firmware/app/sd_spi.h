/* SD card in SPI mode on SPI1, 512-byte sectors. */
#ifndef SD_SPI_H
#define SD_SPI_H

#include <stdbool.h>
#include <stdint.h>

/* Initialise the card. Returns false if no usable card answers. */
bool sd_init(void);

bool sd_read(uint8_t *buffer, uint32_t sector, uint32_t count);
bool sd_write(const uint8_t *buffer, uint32_t sector, uint32_t count);

/* Wait until the card has finished its internal programming. */
bool sd_sync(void);

#endif
