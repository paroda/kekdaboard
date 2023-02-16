/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdio.h>
//
#include "ff15/ff.h" /* Obtains integer types */
//
#include "ff15/diskio.h" /* Declarations of disk functions */

#include "sd_card.h"

sd_card_t* sd_cards[2];
const TCHAR* sd_card_pcNames[2] = {"0:", "1:"};
uint8_t sd_cards_count=0;

uint8_t disk_register_sd_card(sd_card_t* sd) {
    if(sd_cards_count<2) {
        uint8_t id = sd_cards_count++;
        sd_cards[id] = sd;
        sd->pcName = sd_card_pcNames[id];
        return id;
    }
    return -1;
}

static sd_card_t* sd_get_by_num(uint8_t id) {
    if(id<sd_cards_count)
        return sd_cards[id];
    else
        return NULL;
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE pdrv) { /* Physical drive nmuber to identify the drive */
    sd_card_t *p_sd = sd_get_by_num(pdrv);
    if (!p_sd) return RES_PARERR;
    return p_sd->status;  // See http://elm-chan.org/fsw/ff/doc/dstat.html
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE pdrv) {
    /* Physical drive nmuber to identify the drive */
    sd_card_t *p_sd = sd_get_by_num(pdrv);
    if (!p_sd) return RES_PARERR;
    return p_sd->status;  // See http://elm-chan.org/fsw/ff/doc/dstat.html
}

static int sdrc2dresult(int sd_rc) {
    switch (sd_rc) {
    case SD_BLOCK_DEVICE_ERROR_NONE:
        return RES_OK;
    case SD_BLOCK_DEVICE_ERROR_UNUSABLE:
    case SD_BLOCK_DEVICE_ERROR_NO_RESPONSE:
    case SD_BLOCK_DEVICE_ERROR_NO_INIT:
    case SD_BLOCK_DEVICE_ERROR_NO_DEVICE:
        return RES_NOTRDY;
    case SD_BLOCK_DEVICE_ERROR_PARAMETER:
    case SD_BLOCK_DEVICE_ERROR_UNSUPPORTED:
        return RES_PARERR;
    case SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED:
        return RES_WRPRT;
    case SD_BLOCK_DEVICE_ERROR_CRC:
    case SD_BLOCK_DEVICE_ERROR_WOULD_BLOCK:
    case SD_BLOCK_DEVICE_ERROR_ERASE:
    case SD_BLOCK_DEVICE_ERROR_WRITE:
    default:
        return RES_ERROR;
    }
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE pdrv,  /* Physical drive nmuber to identify the drive */
                  BYTE *buff, /* Data buffer to store read data */
                  LBA_t sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
    ) {
    sd_card_t *p_sd = sd_get_by_num(pdrv);
    if (!p_sd) return RES_PARERR;
    int rc = sd_read_blocks(p_sd, buff, sector, count);
    return sdrc2dresult(rc);
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(BYTE pdrv, /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   LBA_t sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
    ) {
    sd_card_t *p_sd = sd_get_by_num(pdrv);
    if (!p_sd) return RES_PARERR;
    int rc = sd_write_blocks(p_sd, buff, sector, count);
    return sdrc2dresult(rc);
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
    ) {
    sd_card_t *p_sd = sd_get_by_num(pdrv);
    if (!p_sd) return RES_PARERR;
    switch (cmd) {
    case GET_SECTOR_COUNT: {  // Retrieves number of available sectors, the
        // largest allowable LBA + 1, on the drive
        // into the LBA_t variable pointed by buff.
        // This command is used by f_mkfs and f_fdisk
        // function to determine the size of
        // volume/partition to be created. It is
        // required when FF_USE_MKFS == 1.
        static LBA_t n;
        n = sd_sectors(p_sd);
        *(LBA_t *)buff = n;
        if (!n) return RES_ERROR;
        return RES_OK;
    }
    case GET_BLOCK_SIZE: {  // Retrieves erase block size of the flash
        // memory media in unit of sector into the DWORD
        // variable pointed by buff. The allowable value
        // is 1 to 32768 in power of 2. Return 1 if the
        // erase block size is unknown or non flash
        // memory media. This command is used by only
        // f_mkfs function and it attempts to align data
        // area on the erase block boundary. It is
        // required when FF_USE_MKFS == 1.
        static DWORD bs = 1;
        *(DWORD *)buff = bs;
        return RES_OK;
    }
    case CTRL_SYNC:
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
