#include "logger.h"

#include <string.h>

#include "csv.h"
#include "ff.h"

static FATFS fs;
static FIL file;
static bool file_open;
static DWORD fat_time = ((DWORD)(2025 - 1980) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16);

static bool next_number(uint32_t *next)
{
    DIR dir;
    FILINFO info;
    uint32_t highest = 0;

    if (f_opendir(&dir, "/") != FR_OK) {
        return false;
    }
    for (;;) {
        uint32_t number;
        const FRESULT result = f_readdir(&dir, &info);

        if (result != FR_OK) {
            (void)f_closedir(&dir);
            return false;
        }
        if (info.fname[0] == '\0') {
            break;
        }
        if (log_name_parse(info.fname, &number) && number > highest) {
            highest = number;
        }
    }
    (void)f_closedir(&dir);

    if (highest >= LOG_NUMBER_MAX) {
        return false;
    }
    *next = highest + 1;
    return true;
}

static bool write_all(const void *data, UINT length)
{
    UINT written = 0;

    /* A short write means the volume is full. */
    return f_write(&file, data, length, &written) == FR_OK && written == length;
}

bool logger_start(void)
{
    uint32_t number;
    char name[LOG_NAME_SIZE];

    if (f_mount(&fs, "", 1) != FR_OK) {
        return false;
    }
    if (!next_number(&number)) {
        logger_abort();
        return false;
    }

    log_name_format(name, number);
    if (f_open(&file, name, FA_WRITE | FA_CREATE_NEW) != FR_OK) {
        logger_abort();
        return false;
    }
    file_open = true;

    if (!write_all(CSV_HEADER, (UINT)strlen(CSV_HEADER)) || !logger_sync()) {
        logger_abort();
        return false;
    }
    return true;
}

bool logger_write(const gps_fix_t *fix)
{
    char row[CSV_ROW_MAX];
    const size_t length = csv_format_row(row, sizeof row, fix);

    if (!file_open || length == 0) {
        return false;
    }

    fat_time = ((DWORD)(fix->year - 1980u) << 25) | ((DWORD)fix->month << 21) |
               ((DWORD)fix->day << 16) | ((DWORD)fix->hour << 11) |
               ((DWORD)fix->minute << 5) | ((DWORD)fix->second >> 1);

    return write_all(row, (UINT)length);
}

bool logger_sync(void)
{
    return file_open && f_sync(&file) == FR_OK;
}

bool logger_stop(void)
{
    bool ok = true;

    if (file_open) {
        ok = f_close(&file) == FR_OK;
        file_open = false;
    }
    (void)f_mount(NULL, "", 0);
    return ok;
}

void logger_abort(void)
{
    file_open = false;
    (void)f_mount(NULL, "", 0);
}

DWORD get_fattime(void)
{
    return fat_time;
}
