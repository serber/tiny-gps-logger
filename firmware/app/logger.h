/* Log files on the FAT volume. Depends only on FatFs, so it runs on a RAM disk in the host tests. */
#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>

#include "ubx.h"

/* Mount the volume and create the next LOGnnnnn.CSV with its header. */
bool logger_start(void);

/* Append one row. FatFs buffers it, so most calls do not touch the card. */
bool logger_write(const gps_fix_t *fix);

/* Commit buffered rows and the directory entry to the card. */
bool logger_sync(void);

/* Close the file and unmount. */
bool logger_stop(void);

/* Forget the file and the volume without touching the card, after removal or an error. */
void logger_abort(void);

#endif
