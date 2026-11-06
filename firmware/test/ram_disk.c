/* FatFs media layer on RAM, for the host tests. */
#include <stdlib.h>
#include <string.h>

#include "ff.h"

#include "diskio.h"
#include "ram_disk.h"

#define SECTOR_SIZE 512u

static uint8_t *image;
static uint32_t sector_count;

void ram_disk_create(uint32_t sectors)
{
    free(image);
    image = calloc(sectors, SECTOR_SIZE);
    sector_count = sectors;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    return (pdrv == 0 && image != NULL) ? 0 : STA_NOINIT;
}

DSTATUS disk_status(BYTE pdrv)
{
    return disk_initialize(pdrv);
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || sector + count > sector_count) {
        return RES_PARERR;
    }
    memcpy(buff, image + (size_t)sector * SECTOR_SIZE, (size_t)count * SECTOR_SIZE);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || sector + count > sector_count) {
        return RES_PARERR;
    }
    memcpy(image + (size_t)sector * SECTOR_SIZE, buff, (size_t)count * SECTOR_SIZE);
    return RES_OK;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0) {
        return RES_PARERR;
    }
    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        *(LBA_t *)buff = sector_count;
        return RES_OK;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 1;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
