#ifndef _SD_CARD_H
#define _SD_CARD_H

#include <stdint.h>
#include <stdbool.h>

#include "hardware/gpio.h"
#include "pico/mutex.h"

#include "ff15/ff.h"

#include "../master_spi.h"

typedef struct {
    master_spi_t* m_spi;
    uint8_t spi_slave_id;
    uint8_t ss_gpio; // CS pin

    uint64_t sectors; // assigned dynamically
    int card_type;    // assigned dynamically

    const char* pcName;  // drive name
    FATFS fatfs; // fatfs.fs_type==0 means unmounted
    int status;  // card status
} sd_card_t;

#define SD_BLOCK_DEVICE_ERROR_NONE 0
#define SD_BLOCK_DEVICE_ERROR_WOULD_BLOCK -5001 /*!< operation would block */
#define SD_BLOCK_DEVICE_ERROR_UNSUPPORTED -5002 /*!< unsupported operation */
#define SD_BLOCK_DEVICE_ERROR_PARAMETER -5003   /*!< invalid parameter */
#define SD_BLOCK_DEVICE_ERROR_NO_INIT -5004     /*!< uninitialized */
#define SD_BLOCK_DEVICE_ERROR_NO_DEVICE -5005   /*!< device is missing or not connected */
#define SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED -5006 /*!< write protected */
#define SD_BLOCK_DEVICE_ERROR_UNUSABLE -5007    /*!< unusable card */
#define SD_BLOCK_DEVICE_ERROR_NO_RESPONSE -5008 /*!< No response from device */
#define SD_BLOCK_DEVICE_ERROR_CRC -5009    /*!< CRC error */
#define SD_BLOCK_DEVICE_ERROR_ERASE -5010 /*!< Erase error: reset/sequence */
#define SD_BLOCK_DEVICE_ERROR_WRITE -5011 /*!< SPI Write error: !SPI_DATA_ACCEPTED */

#define SPI_FILL_CHAR (0xFF)

/*
 * // Disk Status Bits (DSTATUS)
 * See diskio.h.
 * enum {
 *   STA_NOINIT = 0x01, // Drive not initialized
 *   STA_NODISK = 0x02, // No medium in the drive
 *   STA_PROTECT = 0x04 // Write protected
 * };
 */

sd_card_t* sd_create(master_spi_t* m_spi, uint8_t gpio_CS);

void sd_free(sd_card_t* sd);

int sd_write_blocks(sd_card_t *pSD, const uint8_t *buffer, uint64_t ulSectorNumber, uint32_t blockCnt);

int sd_read_blocks(sd_card_t *pSD, uint8_t *buffer, uint64_t ulSectorNumber, uint32_t ulSectorCount);

uint64_t sd_sectors(sd_card_t *pSD);

void sd_print(sd_card_t* sd);

#endif
