/* FatFs media layer for the SD card. */
#include "ff.h"

#include "diskio.h"

#include "board.h"
#include "sd_spi.h"

static DSTATUS status = STA_NOINIT;

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }
    if (!board_card_inserted()) {
        status = STA_NOINIT | STA_NODISK;
    }
    return status;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) {
        return STA_NOINIT;
    }
    if (!board_card_inserted()) {
        status = STA_NOINIT | STA_NODISK;
    } else {
        status = sd_init() ? 0 : STA_NOINIT;
    }
    return status;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0) {
        return RES_PARERR;
    }
    if (status & STA_NOINIT) {
        return RES_NOTRDY;
    }
    return sd_read(buff, sector, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || count == 0) {
        return RES_PARERR;
    }
    if (status & STA_NOINIT) {
        return RES_NOTRDY;
    }
    return sd_write(buff, sector, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    (void)buff;

    if (pdrv != 0) {
        return RES_PARERR;
    }
    if (status & STA_NOINIT) {
        return RES_NOTRDY;
    }
    if (cmd == CTRL_SYNC) {
        return sd_sync() ? RES_OK : RES_ERROR;
    }
    return RES_PARERR;
}
