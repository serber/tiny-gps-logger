#include "csv.h"

const char CSV_HEADER[] = "utc_time,lat,lon,alt_m,speed_mps,course_deg,sats,hdop,fix_type\r\n";

typedef struct {
    char *pos;
    char *end;
    bool ok;
} writer_t;

static void put_char(writer_t *w, char c)
{
    if (w->pos < w->end) {
        *w->pos++ = c;
    } else {
        w->ok = false;
    }
}

static void put_uint(writer_t *w, uint64_t value, unsigned min_digits)
{
    char digits[20];
    unsigned count = 0;

    do {
        digits[count++] = (char)('0' + value % 10);
        value /= 10;
    } while (value != 0);
    while (count < min_digits) {
        digits[count++] = '0';
    }
    while (count > 0) {
        put_char(w, digits[--count]);
    }
}

/* Write value, which has in_decimals decimal places, with out_decimals places.
 * Rounds half away from zero. out_decimals must not exceed in_decimals. */
static void put_fixed(writer_t *w, int64_t value, unsigned in_decimals, unsigned out_decimals)
{
    uint64_t magnitude = (value < 0) ? (uint64_t)(-value) : (uint64_t)value;
    uint64_t unit = 1;

    for (unsigned i = in_decimals; i > out_decimals; i--) {
        unit *= 10;
    }
    magnitude = (magnitude + unit / 2) / unit;

    uint64_t scale = 1;
    for (unsigned i = 0; i < out_decimals; i++) {
        scale *= 10;
    }

    if (value < 0 && magnitude != 0) {
        put_char(w, '-');
    }
    put_uint(w, magnitude / scale, 1);
    if (out_decimals > 0) {
        put_char(w, '.');
        put_uint(w, magnitude % scale, out_decimals);
    }
}

size_t csv_format_row(char *out, size_t capacity, const gps_fix_t *fix)
{
    writer_t w = {out, out + capacity, true};

    put_uint(&w, fix->year, 4);
    put_char(&w, '-');
    put_uint(&w, fix->month, 2);
    put_char(&w, '-');
    put_uint(&w, fix->day, 2);
    put_char(&w, 'T');
    put_uint(&w, fix->hour, 2);
    put_char(&w, ':');
    put_uint(&w, fix->minute, 2);
    put_char(&w, ':');
    put_uint(&w, fix->second, 2);
    put_char(&w, 'Z');
    put_char(&w, ',');
    put_fixed(&w, fix->lat_e7, 7, 7);
    put_char(&w, ',');
    put_fixed(&w, fix->lon_e7, 7, 7);
    put_char(&w, ',');
    put_fixed(&w, fix->hmsl_mm, 3, 1);
    put_char(&w, ',');
    put_fixed(&w, fix->speed_mm_s, 3, 2);
    put_char(&w, ',');
    put_fixed(&w, fix->course_e5, 5, 1);
    put_char(&w, ',');
    put_uint(&w, fix->sats, 1);
    put_char(&w, ',');
    put_fixed(&w, fix->hdop_e2, 2, 2);
    put_char(&w, ',');
    put_uint(&w, fix->fix_type, 1);
    put_char(&w, '\r');
    put_char(&w, '\n');

    return w.ok ? (size_t)(w.pos - out) : 0;
}

bool log_name_parse(const char *name, uint32_t *number)
{
    static const char prefix[] = "LOG";
    static const char suffix[] = ".CSV";
    uint32_t value = 0;
    unsigned i;

    for (i = 0; i < 3; i++) {
        if (name[i] != prefix[i]) {
            return false;
        }
    }
    for (i = 3; i < 8; i++) {
        if (name[i] < '0' || name[i] > '9') {
            return false;
        }
        value = value * 10 + (uint32_t)(name[i] - '0');
    }
    for (i = 0; i < 5; i++) { /* Includes the terminator. */
        if (name[8 + i] != suffix[i]) {
            return false;
        }
    }
    *number = value;
    return true;
}

void log_name_format(char *out, uint32_t number)
{
    writer_t w = {out, out + LOG_NAME_SIZE, true};

    put_char(&w, 'L');
    put_char(&w, 'O');
    put_char(&w, 'G');
    put_uint(&w, number, 5);
    put_char(&w, '.');
    put_char(&w, 'C');
    put_char(&w, 'S');
    put_char(&w, 'V');
    put_char(&w, '\0');
}
