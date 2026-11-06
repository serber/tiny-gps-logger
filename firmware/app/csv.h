/* CSV rows and log file names. No hardware dependencies. */
#ifndef CSV_H
#define CSV_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "ubx.h"

#define CSV_ROW_MAX 128
#define LOG_NAME_SIZE 13 /* "LOG00001.CSV" and the terminator */
#define LOG_NUMBER_MAX 99999u

extern const char CSV_HEADER[];

/* Format one row with its line ending. Returns the length, or 0 if out is too small. */
size_t csv_format_row(char *out, size_t capacity, const gps_fix_t *fix);

/* Parse "LOGnnnnn.CSV". Returns true and the number when name matches exactly. */
bool log_name_parse(const char *name, uint32_t *number);

/* out must hold LOG_NAME_SIZE bytes. number must be 0 to LOG_NUMBER_MAX. */
void log_name_format(char *out, uint32_t number);

#endif
