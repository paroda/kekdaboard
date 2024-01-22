/* Introduction
 *
 * Dereived from
 * https://github.com/carlk3/no-OS-FatFS-SD-SPI-RPi-Pico/blob/master/FatFs_SPI/sd_driver
 *
 * ------------
 * SD and MMC cards support a number of interfaces, but common to them all
 * is one based on SPI. Since we already have the mbed SPI Interface, it will
 * be used for SD cards.
 *
 * The main reference I'm using is Chapter 7, "SPI Mode" of:
 *  http://www.sdcard.org/developers/tech/sdcard/pls/Simplified_Physical_Layer_Spec.pdf
 *
 * SPI Startup
 * -----------
 * The SD card powers up in SD mode. The start-up procedure is complicated
 * by the requirement to support older SDCards in a backwards compatible
 * way with the new higher capacity variants SDHC and SDHC.
 *
 * The following figures from the specification with associated text describe
 * the SPI mode initialisation process:
 *  - Figure 7-1: SD Memory Card State Diagram (SPI mode)
 *  - Figure 7-2: SPI Mode Initialization Flow
 *
 * Firstly, a low initial clock should be selected (in the range of 100-
 * 400kHZ). After initialisation has been completed, the switch to a
 * higher clock speed can be made (e.g. 1MHz). Newer cards will support
 * higher speeds than the default _transfer_sck defined here.
 *
 * Next, note the following from the SDCard specification (note to
 * Figure 7-1):
 *
 *  In any of the cases CMD1 is not recommended because it may be difficult for
 * the host to distinguish between MultiMediaCard and SD Memory Card
 *
 * Hence CMD1 is not used for the initialisation sequence.
 *
 * The SPI interface mode is selected by asserting CS low and sending the
 * reset command (CMD0). The card will respond with a (R1) response.
 * In practice many cards initially respond with 0xff or invalid data
 * which is ignored. Data is read until a valid response is received
 * or the number of re-reads has exceeded a maximim count. If a valid
 * response is not received then the CMD0 can be retried. This
 * has been found to successfully initialise cards where the SPI master
 * (on MCU) has been reset but the SDCard has not, so the first
 * CMD0 may be lost.
 *
 * CMD8 is optionally sent to determine the voltage range supported, and
 * indirectly determine whether it is a version 1.x SD/non-SD card or
 * version 2.x. I'll just ignore this for now.
 *
 * ACMD41 is repeatedly issued to initialise the card, until "in idle"
 * (bit 0) of the R1 response goes to '0', indicating it is initialised.
 *
 * You should also indicate whether the host supports High Capicity cards,
 * and check whether the card is high capacity - i'll also ignore this.
 *
 * SPI Protocol
 * ------------
 * The SD SPI protocol is based on transactions made up of 8-bit words, with
 * the host starting every bus transaction by asserting the CS signal low. The
 * card always responds to commands, data blocks and errors.
 *
 * The protocol supports a CRC, but by default it is off (except for the
 * first reset CMD0, where the CRC can just be pre-calculated, and CMD8)
 * I'll leave the CRC off I think!
 *
 * Standard capacity cards have variable data block sizes, whereas High
 * Capacity cards fix the size of data block to 512 bytes. I'll therefore
 * just always use the Standard Capacity cards with a block size of 512 bytes.
 * This is set with CMD16.
 *
 * You can read and write single blocks (CMD17, CMD25) or multiple blocks
 * (CMD18, CMD25). For simplicity, I'll just use single block accesses. When
 * the card gets a read command, it responds with a response token, and then
 * a data token or an error.
 *
 * SPI Command Format
 * ------------------
 * Commands are 6-bytes long, containing the command, 32-bit argument, and CRC.
 *
 * +---------------+------------+------------+-----------+----------+--------------+
 * | 01 | cmd[5:0] | arg[31:24] | arg[23:16] | arg[15:8] | arg[7:0] | crc[6:0] | 1 |
 * +---------------+------------+------------+-----------+----------+--------------+
 *
 * As I'm not using CRC, I can fix that byte to what is needed for CMD0 (0x95)
 *
 * All Application Specific commands shall be preceded with APP_CMD (CMD55).
 *
 * SPI Response Format
 * -------------------
 * The main response format (R1) is a status byte (normally zero). Key flags:
 *  idle - 1 if the card is in an idle state/initialising
 *  cmd  - 1 if an illegal command code was detected
 *
 *    +-------------------------------------------------+
 * R1 | 0 | arg | addr | seq | crc | cmd | erase | idle |
 *    +-------------------------------------------------+
 *
 * R1b is the same, except it is followed by a busy signal (zeros) until
 * the first non-zero byte when it is ready again.
 *
 * Data Response Token
 * -------------------
 * Every data block written to the card is acknowledged by a byte
 * response token
 *
 * +----------------------+
 * | xxx | 0 | status | 1 |
 * +----------------------+
 *              010 - OK!
 *              101 - CRC Error
 *              110 - Write Error
 *
 * Single Block Read and Write
 * ---------------------------
 *
 * Block transfers have a byte header, followed by the data, followed
 * by a 16-bit CRC. In our case, the data will always be 512 bytes.
 *
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 * | 0xFE | data[0] | data[1] |        | data[n] | crc[15:8] | crc[7:0] |
 * +------+---------+---------+- -  - -+---------+-----------+----------+
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pico/stdlib.h>

#include "hardware/gpio.h"

#include "sd_card.h"

#include "ff15/ff.h"
#include "ff15/diskio.h"

#ifndef SD_CRC_ENABLED
#define SD_CRC_ENABLED 1
#endif

#if SD_CRC_ENABLED
#include "crc.h"
static bool crc_on = true;
#endif

#define DBG_PRINTF printf

/* Control Tokens */
#define SPI_DATA_RESPONSE_MASK (0x1F)
#define SPI_DATA_ACCEPTED (0x05)
#define SPI_DATA_CRC_ERROR (0x0B)
#define SPI_DATA_WRITE_ERROR (0x0D)
#define SPI_START_BLOCK (0xFE)         /* For Single Block Read/Write and Multiple Block Read */
#define SPI_START_BLK_MUL_WRITE (0xFC) /* Start Multi-block write */
#define SPI_STOP_TRAN (0xFD)           /* Stop Multi-block write */

#define SPI_DATA_READ_ERROR_MASK (0xF)  /* Data Error Token: 4 LSB bits */
#define SPI_READ_ERROR (0x1 << 0)       /* Error */
#define SPI_READ_ERROR_CC (0x1 << 1)    /* CC Error*/
#define SPI_READ_ERROR_ECC_C (0x1 << 2) /* Card ECC failed */
#define SPI_READ_ERROR_OFR (0x1 << 3)   /* Out of Range */

#define SDCARD_MODE 0
#define SDCARD_BAUD (25 * 1000 * 1000)

/* Represents the different SD/MMC card types  */
// Types
#define SDCARD_NONE 0  /* No card is present */
#define SDCARD_V1 1    /* v1.x Standard Capacity */
#define SDCARD_V2 2    /* v2.x Standard capacity SD card */
#define SDCARD_V2HC 3  /* v2.x High capacity SD card */
#define CARD_UNKNOWN 4 /* Unknown or unsupported card */

// Only HC block size is supported. Making this a static constant reduces code size.
#define BLOCK_SIZE_HC  512 /*!< Block size supported for SD card is 512 bytes */
static const uint32_t _block_size = BLOCK_SIZE_HC;

/* R1 Response Format */
#define R1_NO_RESPONSE (0xFF)
#define R1_RESPONSE_RECV (0x80)
#define R1_IDLE_STATE (1 << 0)
#define R1_ERASE_RESET (1 << 1)
#define R1_ILLEGAL_COMMAND (1 << 2)
#define R1_COM_CRC_ERROR (1 << 3)
#define R1_ERASE_SEQUENCE_ERROR (1 << 4)
#define R1_ADDRESS_ERROR (1 << 5)
#define R1_PARAMETER_ERROR (1 << 6)

// Supported SD Card Commands
typedef enum {
    CMD_NOT_SUPPORTED = -1,         /* Command not supported error */
    CMD0_GO_IDLE_STATE = 0,         /* Resets the SD Memory Card */
    CMD1_SEND_OP_COND = 1,          /* Sends host capacity support */
    CMD6_SWITCH_FUNC = 6,           /* Check and Switches card function */
    CMD8_SEND_IF_COND = 8,          /* Supply voltage info */
    CMD9_SEND_CSD = 9,              /* Provides Card Specific data */
    CMD10_SEND_CID = 10,            /* Provides Card Identification */
    CMD12_STOP_TRANSMISSION = 12,   /* Forces the card to stop transmission */
    CMD13_SEND_STATUS = 13,         /* Card responds with status */
    CMD16_SET_BLOCKLEN = 16,        /* Length for SC card is set */
    CMD17_READ_SINGLE_BLOCK = 17,   /* Read single block of data */
    CMD18_READ_MULTIPLE_BLOCK = 18, /* Card transfers data blocks to host
                                       until interrupted by a STOP_TRANSMISSION command */
    CMD24_WRITE_BLOCK = 24,         /* Write single block of data */
    CMD25_WRITE_MULTIPLE_BLOCK = 25,    /* Continuously writes blocks of data
                                           until    'Stop Tran' token is sent */
    CMD27_PROGRAM_CSD = 27,             /* Programming bits of CSD */
    CMD32_ERASE_WR_BLK_START_ADDR = 32, /* Sets the address of the first write
                                           block to be erased. */
    CMD33_ERASE_WR_BLK_END_ADDR = 33,   /* Sets the address of the last write
                                           block of the continuous range to be erased.*/
    CMD38_ERASE = 38,      /* Erases all previously selected write blocks */
    CMD55_APP_CMD = 55,    /* Extend to Applications specific commands */
    CMD56_GEN_CMD = 56,    /* General Purpose Command */
    CMD58_READ_OCR = 58,   /* Read OCR register of card */
    CMD59_CRC_ON_OFF = 59, /* Turns the CRC option on or off*/
    // App Commands
    ACMD6_SET_BUS_WIDTH = 6,
    ACMD13_SD_STATUS = 13,
    ACMD22_SEND_NUM_WR_BLOCKS = 22,
    ACMD23_SET_WR_BLK_ERASE_COUNT = 23,
    ACMD41_SD_SEND_OP_COND = 41,
    ACMD42_SET_CLR_CARD_DETECT = 42,
    ACMD51_SEND_SCR = 51,
} cmdSupported;

/* SIZE in Bytes */
#define PACKET_SIZE 6         /* SD Packet size CMD+ARG+CRC */
#define R1_RESPONSE_SIZE 1    /* Size of R1 response */
#define R2_RESPONSE_SIZE 2    /* Size of R2 response */
#define R3_R7_RESPONSE_SIZE 5 /* Size of R3/R7 response */

/* R3 Response : OCR Register */
#define OCR_HCS_CCS (0x1 << 30)
#define OCR_LOW_VOLTAGE (0x01 << 24)
#define OCR_3_3V (0x1 << 20)

#define SPI_CMD(x) (0x40 | (x & 0x3f))

/*
 * SD SPI Operations
 */

static void sd_spi_select(sd_card_t *pSD) {
    master_spi_select_slave(pSD->m_spi, pSD->spi_slave_id);
    // A fill byte seems to be necessary, sometimes:
    uint8_t fill = SPI_FILL_CHAR;
    master_spi_write8(pSD->m_spi, &fill, 1);
}

static void sd_spi_deselect(sd_card_t* pSD) {
    master_spi_release_slave(pSD->m_spi, pSD->spi_slave_id);
    /*
      MMC/SDC enables/disables the DO output in synchronising to the SCLK. This
      means there is a posibility of bus conflict with MMC/SDC and another SPI
      slave that shares an SPI bus. Therefore to make MMC/SDC release the MISO
      line, the master device needs to send a byte after the CS signal is
      deasserted.
    */
    uint8_t fill = SPI_FILL_CHAR;
    master_spi_write8(pSD->m_spi, &fill, 1);
}

/* Some SD cards want to be deselected between every bus transaction */
static void sd_spi_deselect_pulse(sd_card_t *pSD) {
    sd_spi_deselect(pSD);
    // tCSH Pulse duration, CS high 200 ns
    sd_spi_select(pSD);
}

static uint8_t sd_spi_write(sd_card_t *pSD, const uint8_t value) {
    uint8_t received = SPI_FILL_CHAR;
    master_spi_write8_read8(pSD->m_spi, &value, &received, 1);
    return received;
}

static void sd_spi_send_initializing_sequence(sd_card_t * pSD) {
    bool old_ss = gpio_get(pSD->ss_gpio);
    // Set DI and CS high and apply 74 or more clock pulses to SCLK:
    gpio_put(pSD->ss_gpio, 1);
    uint8_t ones[10];
    memset(ones, 0xFF, sizeof ones);
    absolute_time_t timeout_time = make_timeout_time_ms(1);
    do {
        master_spi_write8_read8(pSD->m_spi, ones, NULL, sizeof ones);
    } while (0 < absolute_time_diff_us(get_absolute_time(), timeout_time));
    gpio_put(pSD->ss_gpio, old_ss);
}


/*
 * SD CARD Operations
 */

static uint8_t sd_cmd_spi(sd_card_t *pSD, cmdSupported cmd, uint32_t arg) {
    uint8_t response;
    char cmdPacket[PACKET_SIZE];

    // Prepare the command packet
    cmdPacket[0] = SPI_CMD(cmd);
    cmdPacket[1] = (arg >> 24);
    cmdPacket[2] = (arg >> 16);
    cmdPacket[3] = (arg >> 8);
    cmdPacket[4] = (arg >> 0);

#if SD_CRC_ENABLED
    if (crc_on) {
        cmdPacket[5] = (crc7(cmdPacket, 5) << 1) | 0x01;
    } else
#endif
    {
        // CMD0 is executed in SD mode, hence should have correct CRC
        // CMD8 CRC verification is always enabled
        switch (cmd) {
        case CMD0_GO_IDLE_STATE:
            cmdPacket[5] = 0x95;
            break;
        case CMD8_SEND_IF_COND:
            cmdPacket[5] = 0x87;
            break;
        default:
            cmdPacket[5] = 0xFF;  // Make sure bit 0-End bit is high
            break;
        }
    }
    // send a command
    for (int i = 0; i < PACKET_SIZE; i++) {
        sd_spi_write(pSD, cmdPacket[i]);
    }
    // The received byte immediataly following CMD12 is a stuff byte,
    // it should be discarded before receive the response of the CMD12.
    if (CMD12_STOP_TRANSMISSION == cmd) {
        sd_spi_write(pSD, SPI_FILL_CHAR);
    }
    // Loop for response: Response is sent back within command response time
    // (NCR), 0 to 8 bytes for SDC
    for (int i = 0; i < 0x10; i++) {
        response = sd_spi_write(pSD, SPI_FILL_CHAR);
        // Got the response
        if (!(response & R1_RESPONSE_RECV)) {
            break;
        }
    }
    return response;
}

static bool sd_wait_ready(sd_card_t *pSD, int timeout) {
    char resp;
    // Keep sending dummy clocks with DI held high until the card releases the
    // DO line
    absolute_time_t timeout_time = make_timeout_time_ms(timeout);
    do {
        resp = sd_spi_write(pSD, 0xFF);
    } while (resp == 0x00 && 0 < absolute_time_diff_us(get_absolute_time(), timeout_time));
    // Return success/failure
    return (resp > 0x00);
}

#define SD_COMMAND_TIMEOUT 2000 /* Timeout in ms for response */

static int sd_cmd(sd_card_t *pSD, const cmdSupported cmd, uint32_t arg, bool isAcmd, uint32_t *resp) {
    int32_t status = SD_BLOCK_DEVICE_ERROR_NONE;
    uint32_t response;

    // No need to wait for card to be ready when sending the stop command
    if (CMD12_STOP_TRANSMISSION != cmd) {
        sd_wait_ready(pSD, SD_COMMAND_TIMEOUT);
    }
    // Re-try command
    for (int i = 0; i < 3; i++) {
        // Send CMD55 for APP command first
        if (isAcmd) {
            response = sd_cmd_spi(pSD, CMD55_APP_CMD, 0x0);
            // Wait for card to be ready after CMD55
            sd_wait_ready(pSD, SD_COMMAND_TIMEOUT);
        }
        // Send command over SPI interface
        response = sd_cmd_spi(pSD, cmd, arg);
        if (R1_NO_RESPONSE != response) break;
    }
    // Pass the response to the command call if required
    if (NULL != resp) {
        *resp = response;
    }
    // Process the response R1  : Exit on CRC/Illegal command error/No response
    if (R1_NO_RESPONSE == response) {
        return SD_BLOCK_DEVICE_ERROR_NO_DEVICE;  // No device
    }
    if (response & R1_COM_CRC_ERROR && ACMD23_SET_WR_BLK_ERASE_COUNT != cmd) {
        return SD_BLOCK_DEVICE_ERROR_CRC;  // CRC error
    }
    if (response & R1_ILLEGAL_COMMAND) {
        if (CMD8_SEND_IF_COND == cmd) {
            // Illegal command is for Ver1 or not SD Card
            pSD->card_type = CARD_UNKNOWN;
        }
        return SD_BLOCK_DEVICE_ERROR_UNSUPPORTED;  // Command not supported
    }

    // Set status for other errors
    if ((response & R1_ERASE_RESET) || (response & R1_ERASE_SEQUENCE_ERROR)) {
        status = SD_BLOCK_DEVICE_ERROR_ERASE;  // Erase error
    } else if ((response & R1_ADDRESS_ERROR) ||
               (response & R1_PARAMETER_ERROR)) {
        // Misaligned address / invalid address block length
        status = SD_BLOCK_DEVICE_ERROR_PARAMETER;
    }

    // Get rest of the response part for other commands
    switch (cmd) {
    case CMD8_SEND_IF_COND:  // Response R7
        pSD->card_type = SDCARD_V2;  // fallthrough
        // Note: No break here, need to read rest of the response
    case CMD58_READ_OCR:  // Response R3
        response = (sd_spi_write(pSD, SPI_FILL_CHAR) << 24);
        response |= (sd_spi_write(pSD, SPI_FILL_CHAR) << 16);
        response |= (sd_spi_write(pSD, SPI_FILL_CHAR) << 8);
        response |= sd_spi_write(pSD, SPI_FILL_CHAR);
        break;
    case CMD12_STOP_TRANSMISSION:  // Response R1b
    case CMD38_ERASE:
        sd_wait_ready(pSD, SD_COMMAND_TIMEOUT);
        break;
    case CMD13_SEND_STATUS:  // Response R2
        response <<= 8;
        response |= sd_spi_write(pSD, SPI_FILL_CHAR);
        if (response) {
            /* DBG_PRINTF("R2: 0x%" PRIx32 "\r\n", response); */
            if (response & 0x01 << 0) {
                /* DBG_PRINTF("Card is Locked                         \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE;
            }
            if (response & 0x01 << 1) {
                /* DBG_PRINTF("WP Erase Skip, Lock/Unlock Cmd Failed  \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED;
            }
            if (response & 0x01 << 2) {
                /* DBG_PRINTF("Error                                  \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE;
            }
            if (response & 0x01 << 3) {
                /* DBG_PRINTF("CC Error                               \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE;
            }
            if (response & 0x01 << 4) {
                /* DBG_PRINTF("Card ECC Failed                        \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE;
            }
            if (response & 0x01 << 5) {
                /* DBG_PRINTF("WP Violation                           \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE_PROTECTED;
            }
            if (response & 0x01 << 6) {
                /* DBG_PRINTF("Erase Param                            \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_ERASE;
            }
            if (response & 0x01 << 7) {
                /* DBG_PRINTF("Out of Range, CSD_Overwrite            \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_PARAMETER;
            }
            if (response & 0x01 << 8) {
                /* DBG_PRINTF("In Idle State                          \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_NONE;
            }
            if (response & 0x01 << 9) {
                /* DBG_PRINTF("Erase Reset                            \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_ERASE;
            }
            if (response & 0x01 << 10) {
                /* DBG_PRINTF("Illegal Command                        \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_UNSUPPORTED;
            }
            if (response & 0x01 << 11) {
                /* DBG_PRINTF("Com CRC Error                          \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_CRC;
            }
            if (response & 0x01 << 12) {
                /* DBG_PRINTF("Erase Sequence Error                   \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_ERASE;
            }
            if (response & 0x01 << 13) {
                /* DBG_PRINTF("Address Error                          \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_PARAMETER;
            }
            if (response & 0x01 << 14) {
                /* DBG_PRINTF("Parameter Error                        \r\n"); */
                status = SD_BLOCK_DEVICE_ERROR_PARAMETER;
            }
            break;
        }
    default:  // Response R1
        break;
    }
    // Pass the updated response to the command
    if (NULL != resp) {
        *resp = response;
    }
    return status;
}

/* Number of retries for sending CMDO */
#define SD_CMD0_GO_IDLE_STATE_RETRIES 10

static uint32_t sd_go_idle_state(sd_card_t *pSD) {
    uint32_t response;
    /* Resetting the MCU SPI master may not reset the on-board SDCard, in which
     * case when MCU power-on occurs the SDCard will resume operations as
     * though there was no reset. In this scenario the first CMD0 will
     * not be interpreted as a command and get lost. For some cards retrying
     * the command overcomes this situation. */
    for (int i = 0; i < SD_CMD0_GO_IDLE_STATE_RETRIES; i++) {
        sd_cmd(pSD, CMD0_GO_IDLE_STATE, 0x0, false, &response);
        if (R1_IDLE_STATE == response) {
            break;
        }
        sd_spi_deselect(pSD);
        busy_wait_us(100 * 1000);
        sd_spi_select(pSD);
    }
    return response;
}

/* R7 response pattern for CMD8 */
#define CMD8_PATTERN (0xAA)

static int sd_cmd8(sd_card_t *pSD) {
    uint32_t arg = (CMD8_PATTERN << 0);  // [7:0]check pattern
    uint32_t response = 0;
    int32_t status = SD_BLOCK_DEVICE_ERROR_NONE;

    arg |= (0x1 << 8);  // 2.7-3.6V             // [11:8]supply voltage(VHS)

    status = sd_cmd(pSD, CMD8_SEND_IF_COND, arg, false, &response);
    // Verify voltage and pattern for V2 version of card
    if ((SD_BLOCK_DEVICE_ERROR_NONE == status) &&
        (SDCARD_V2 == pSD->card_type)) {
        // If check pattern is not matched, CMD8 communication is not valid
        if ((response & 0xFFF) != arg) {
            pSD->card_type = CARD_UNKNOWN;
            status = SD_BLOCK_DEVICE_ERROR_UNUSABLE;
        }
    }
    return status;
}

static uint32_t ext_bits(unsigned char *data, int msb, int lsb) {
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = 15 - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}

// SPI function to wait till chip is ready and sends start token
static bool sd_wait_token(sd_card_t *pSD, uint8_t token) {
    const uint32_t timeout = SD_COMMAND_TIMEOUT;  // Wait for start token
    absolute_time_t timeout_time = make_timeout_time_ms(timeout);
    do {
        if (token == sd_spi_write(pSD, SPI_FILL_CHAR)) {
            return true;
        }
    } while (0 < absolute_time_diff_us(get_absolute_time(), timeout_time));
    /* DBG_PRINTF("sd_wait_token: timeout\r\n"); */
    return false;
}

#define SPI_START_BLOCK (0xFE) /* For Single Block Read/Write and Multiple Block Read */

static int sd_read_bytes(sd_card_t *pSD, uint8_t *buffer, uint32_t length) {
    uint16_t crc;
    // read until start byte (0xFE)
    if (false == sd_wait_token(pSD, SPI_START_BLOCK)) {
        /* DBG_PRINTF("%s:%d Read timeout\r\n", __FILE__, __LINE__); */
        return SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
    }
    // read data
    for (uint32_t i = 0; i < length; i++) {
        buffer[i] = sd_spi_write(pSD, SPI_FILL_CHAR);
    }
    // Read the CRC16 checksum for the data block
    crc = (sd_spi_write(pSD, SPI_FILL_CHAR) << 8);
    crc |= sd_spi_write(pSD, SPI_FILL_CHAR);
#if SD_CRC_ENABLED
    if (crc_on) {
        uint32_t crc_result;
        // Compute and verify checksum
        crc_result = crc16((void *)buffer, length);
        if ((uint16_t)crc_result != crc) {
            /* DBG_PRINTF("_read_bytes: Invalid CRC received 0x%" PRIx16 " result of computation 0x%" PRIx16 "\r\n", crc, (uint16_t)crc_result); */
            return SD_BLOCK_DEVICE_ERROR_CRC;
        }
    }
#endif
    return 0;
}

static int sd_read_block(sd_card_t *pSD, uint8_t *buffer, uint32_t length) {
    uint16_t crc;
    // read until start byte (0xFE)
    if (false == sd_wait_token(pSD, SPI_START_BLOCK)) {
        /* DBG_PRINTF("%s:%d Read timeout\r\n", __FILE__, __LINE__); */
        return SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
    }
    // read data
    master_spi_write8_read8(pSD->m_spi, NULL, buffer, length);

    // Read the CRC16 checksum for the data block
    crc = (sd_spi_write(pSD, SPI_FILL_CHAR) << 8);
    crc |= sd_spi_write(pSD, SPI_FILL_CHAR);
#if SD_CRC_ENABLED
    if (crc_on) {
        uint32_t crc_result;
        // Compute and verify checksum
        crc_result = crc16((void *)buffer, length);
        if ((uint16_t)crc_result != crc) {
            /* DBG_PRINTF("%s: Invalid CRC received 0x%" PRIx16 " result of computation 0x%" PRIx16 "\r\n", __FUNCTION__, crc, (uint16_t)crc_result); */
            return SD_BLOCK_DEVICE_ERROR_CRC;
        }
    }
#endif
    return SD_BLOCK_DEVICE_ERROR_NONE;
}

static int in_sd_read_blocks(sd_card_t *pSD, uint8_t *buffer,
                             uint64_t ulSectorNumber, uint32_t ulSectorCount) {
    uint32_t blockCnt = ulSectorCount;
    if (ulSectorNumber + blockCnt > pSD->sectors)
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    if (pSD->status & (STA_NOINIT | STA_NODISK))
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    int status = SD_BLOCK_DEVICE_ERROR_NONE;
    uint64_t addr;
    // SDSC Card (CCS=0) uses byte unit address
    // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit)
    if (SDCARD_V2HC == pSD->card_type) {
        addr = ulSectorNumber;
    } else {
        addr = ulSectorNumber * _block_size;
    }
    // Write command ro receive data
    if (blockCnt > 1) {
        status = sd_cmd(pSD, CMD18_READ_MULTIPLE_BLOCK, addr, false, 0);
    } else {
        status = sd_cmd(pSD, CMD17_READ_SINGLE_BLOCK, addr, false, 0);
    }
    if (SD_BLOCK_DEVICE_ERROR_NONE != status) {
        return status;
    }
    // receive the data : one block at a time
    int rd_status = 0;
    while (blockCnt) {
        if (0 != sd_read_block(pSD, buffer, _block_size)) {
            rd_status = SD_BLOCK_DEVICE_ERROR_NO_RESPONSE;
            break;
        }
        buffer += _block_size;
        --blockCnt;
    }
    // Send CMD12(0x00000000) to stop the transmission for multi-block transfer
    if (ulSectorCount > 1) {
        status = sd_cmd(pSD, CMD12_STOP_TRANSMISSION, 0x0, false, 0);
    }
    return rd_status ? rd_status : status;
}

int sd_read_blocks(sd_card_t *pSD, uint8_t *buffer, uint64_t ulSectorNumber, uint32_t ulSectorCount) {
    sd_spi_select(pSD);
    /* TRACE_PRINTF("sd_read_blocks(0x%p, 0x%llx, 0x%lx)\r\n", buffer, ulSectorNumber, ulSectorCount); */
    int status = in_sd_read_blocks(pSD, buffer, ulSectorNumber, ulSectorCount);
    sd_spi_deselect(pSD);
    return status;
}

static uint8_t sd_write_block(sd_card_t *pSD, const uint8_t *buffer, uint8_t token, uint32_t length) {
    uint16_t crc = (~0);
    uint8_t response = 0xFF;

    // indicate start of block
    sd_spi_write(pSD, token);

    // write the data
    master_spi_write8_read8(pSD->m_spi, buffer, NULL, length);

#if SD_CRC_ENABLED
    if (crc_on) {
        // Compute CRC
        crc = crc16((void *)buffer, length);
    }
#endif

    // write the checksum CRC16
    sd_spi_write(pSD, crc >> 8);
    sd_spi_write(pSD, crc);

    // check the response token
    response = sd_spi_write(pSD, SPI_FILL_CHAR);

    // Wait for last block to be written
    sd_wait_ready(pSD, SD_COMMAND_TIMEOUT);

    return (response & SPI_DATA_RESPONSE_MASK);
}

/** Program blocks to a block device
 *
 *
 *  @param buffer       Buffer of data to write to blocks
 *  @param ulSectorNumber     Logical Address of block to begin writing to (LBA)
 *  @param blockCnt     Size to write in blocks
 *  @return SD_BLOCK_DEVICE_ERROR_NONE(0) - success
 *          SD_BLOCK_DEVICE_ERROR_NO_DEVICE - device (SD card) is missing or not connected
 *          SD_BLOCK_DEVICE_ERROR_CRC - crc error
 *          SD_BLOCK_DEVICE_ERROR_PARAMETER - invalid parameter
 *          SD_BLOCK_DEVICE_ERROR_UNSUPPORTED - unsupported command
 *          SD_BLOCK_DEVICE_ERROR_NO_INIT - device is not initialized
 *          SD_BLOCK_DEVICE_ERROR_WRITE - SPI write error
 *          SD_BLOCK_DEVICE_ERROR_ERASE - erase error
 */
static int in_sd_write_blocks(sd_card_t *pSD, const uint8_t *buffer,
                              uint64_t ulSectorNumber, uint32_t blockCnt) {
    if (ulSectorNumber + blockCnt > pSD->sectors)
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;
    if (pSD->status & (STA_NOINIT | STA_NODISK))
        return SD_BLOCK_DEVICE_ERROR_PARAMETER;

    int status = SD_BLOCK_DEVICE_ERROR_NONE;
    uint8_t response;
    uint64_t addr;

    // SDSC Card (CCS=0) uses byte unit address
    // SDHC and SDXC Cards (CCS=1) use block unit address (512 Bytes unit)
    if (SDCARD_V2HC == pSD->card_type) {
        addr = ulSectorNumber;
    } else {
        addr = ulSectorNumber * _block_size;
    }
    // Send command to perform write operation
    if (blockCnt == 1) {
        // Single block write command
        if (SD_BLOCK_DEVICE_ERROR_NONE !=
            (status = sd_cmd(pSD, CMD24_WRITE_BLOCK, addr, false, 0))) {
            return status;
        }
        // Write data
        response = sd_write_block(pSD, buffer, SPI_START_BLOCK, _block_size);

        // Only CRC and general write error are communicated via response token
        if (response != SPI_DATA_ACCEPTED) {
            /* DBG_PRINTF("Single Block Write failed: 0x%x \r\n", response); */
            status = SD_BLOCK_DEVICE_ERROR_WRITE;
        }
    } else {
        // Pre-erase setting prior to multiple block write operation
        sd_cmd(pSD, ACMD23_SET_WR_BLK_ERASE_COUNT, blockCnt, 1, 0);

        // Some SD cards want to be deselected between every bus transaction:
        sd_spi_deselect_pulse(pSD);

        // Multiple block write command
        if (SD_BLOCK_DEVICE_ERROR_NONE !=
            (status = sd_cmd(pSD, CMD25_WRITE_MULTIPLE_BLOCK, addr, false, 0))) {
            return status;
        }
        // Write the data: one block at a time
        do {
            response = sd_write_block(pSD, buffer, SPI_START_BLK_MUL_WRITE, _block_size);
            if (response != SPI_DATA_ACCEPTED) {
                /* DBG_PRINTF("Multiple Block Write failed: 0x%x\r\n", response); */
                status = SD_BLOCK_DEVICE_ERROR_WRITE;
                break;
            }
            buffer += _block_size;
        } while (--blockCnt);  // Send all blocks of data
        /* In a Multiple Block write operation, the stop transmission will be
         * done by sending 'Stop Tran' token instead of 'Start Block' token at
         * the beginning of the next block
         */
        sd_spi_write(pSD, SPI_STOP_TRAN);
    }
    uint32_t stat = 0;
    // Some SD cards want to be deselected between every bus transaction:
    sd_spi_deselect_pulse(pSD);
    status = sd_cmd(pSD, CMD13_SEND_STATUS, 0, false, &stat);
    return status;
}

int sd_write_blocks(sd_card_t *pSD, const uint8_t *buffer, uint64_t ulSectorNumber, uint32_t blockCnt) {
    sd_spi_select(pSD);
    /* TRACE_PRINTF("sd_write_blocks(0x%p, 0x%llx, 0x%lx)\r\n", buffer, ulSectorNumber, blockCnt); */
    int status = in_sd_write_blocks(pSD, buffer, ulSectorNumber, blockCnt);
    sd_spi_deselect(pSD);
    return status;
}

static int sd_init_medium(sd_card_t *pSD) {
    int32_t status = SD_BLOCK_DEVICE_ERROR_NONE;
    uint32_t response, arg;
    /*
    Power ON or card insersion
    After supply voltage reached above 2.2 volts,
    wait for one millisecond at least.
    Set SPI clock rate between 100 kHz and 400 kHz.
    Set DI and CS high and apply 74 or more clock pulses to SCLK.
    The card will enter its native operating mode and go ready to accept native
    command.
    */
    spi_set_baudrate(pSD->m_spi->spi, 400 * 1000); // Actual frequency: 398089
    sd_spi_send_initializing_sequence(pSD);

    // The card is transitioned from SDCard mode to SPI mode by sending the CMD0
    // + CS Asserted("0")
    if (sd_go_idle_state(pSD) != R1_IDLE_STATE) {
        /* DBG_PRINTF("No disk, or could not put SD card in to SPI idle state\r\n"); */
        return SD_BLOCK_DEVICE_ERROR_NO_DEVICE;
    }

    // Send CMD8, if the card rejects the command then it's probably using the
    // legacy protocol, or is a MMC, or just flat-out broken
    status = sd_cmd8(pSD);
    if (SD_BLOCK_DEVICE_ERROR_NONE != status &&
        SD_BLOCK_DEVICE_ERROR_UNSUPPORTED != status) {
        return status;
    }

#if SD_CRC_ENABLED
    if (crc_on) {
        // Enable CRC
        // int sd_cmd(sd_card_t *pSD, cmdSupported cmd, uint32_t arg, bool
        // isAcmd, uint32_t *resp)
        status = sd_cmd(pSD, CMD59_CRC_ON_OFF, 1, false, 0);
    }
#endif

    // Read OCR - CMD58 Response contains OCR register
    if (SD_BLOCK_DEVICE_ERROR_NONE !=
        (status = sd_cmd(pSD, CMD58_READ_OCR, 0x0, false, &response))) {
        return status;
    }
    // Check if card supports voltage range: 3.3V
    if (!(response & OCR_3_3V)) {
        pSD->card_type = CARD_UNKNOWN;
        status = SD_BLOCK_DEVICE_ERROR_UNUSABLE;
        return status;
    }

    // HCS is set 1 for HC/XC capacity cards for ACMD41, if supported
    arg = 0x0;
    if (SDCARD_V2 == pSD->card_type) {
        arg |= OCR_HCS_CCS;
    }

    /* Idle state bit in the R1 response of ACMD41 is used by the card to inform
     * the host if initialization of ACMD41 is completed. "1" indicates that the
     * card is still initializing. "0" indicates completion of initialization.
     * The host repeatedly issues ACMD41 until this bit is set to "0".
     */
    absolute_time_t timeout_time = make_timeout_time_ms(SD_COMMAND_TIMEOUT);
    do {
        status = sd_cmd(pSD, ACMD41_SD_SEND_OP_COND, arg, true, &response);
    } while (response & R1_IDLE_STATE &&
             0 < absolute_time_diff_us(get_absolute_time(), timeout_time));

    // Initialization complete: ACMD41 successful
    if ((SD_BLOCK_DEVICE_ERROR_NONE != status) || (0x00 != response)) {
        pSD->card_type = CARD_UNKNOWN;
        /* DBG_PRINTF("Timeout waiting for card\r\n"); */
        return status;
    }

    if (SDCARD_V2 == pSD->card_type) {
        // Get the card capacity CCS: CMD58
        if (SD_BLOCK_DEVICE_ERROR_NONE ==
            (status = sd_cmd(pSD, CMD58_READ_OCR, 0x0, false, &response))) {
            // High Capacity card
            if (response & OCR_HCS_CCS) {
                pSD->card_type = SDCARD_V2HC;
                /* DBG_PRINTF("Card Initialized: High Capacity Card\r\n"); */
            } else {
                /* DBG_PRINTF("Card Initialized: Standard Capacity Card: Version 2.x\r\n"); */
            }
        }
    } else {
        pSD->card_type = SDCARD_V1;
        /* DBG_PRINTF("Card Initialized: Version 1.x Card\r\n"); */
    }

#if SD_CRC_ENABLED
    if (!crc_on) {
        // Disable CRC
        status = sd_cmd(pSD, CMD59_CRC_ON_OFF, 0, false, 0);
    }
#else
    status = sd_cmd(pSD, CMD59_CRC_ON_OFF, 0, false, 0);
#endif

    return status;
}

static uint64_t sd_sectors_nolock(sd_card_t *pSD) {
    uint32_t c_size, c_size_mult, read_bl_len;
    uint32_t block_len, mult, blocknr;
    uint32_t hc_c_size;
    uint64_t blocks = 0, capacity = 0;

    // CMD9, Response R2 (R1 byte + 16-byte block read)
    if (sd_cmd(pSD, CMD9_SEND_CSD, 0x0, false, 0) != 0x0) {
        /* DBG_PRINTF("Didn't get a response from the disk\r\n"); */
        return 0;
    }
    uint8_t csd[16];
    if (sd_read_bytes(pSD, csd, 16) != 0) {
        /* DBG_PRINTF("Couldn't read csd response from disk\r\n"); */
        return 0;
    }
    // csd_structure : csd[127:126]
    int csd_structure = ext_bits(csd, 127, 126);
    switch (csd_structure) {
    case 0:
        c_size = ext_bits(csd, 73, 62);       // c_size        : csd[73:62]
        c_size_mult = ext_bits(csd, 49, 47);  // c_size_mult   : csd[49:47]
        read_bl_len = ext_bits(csd, 83, 80);  // read_bl_len   : csd[83:80] - the
        // *maximum* read block length
        block_len = 1 << read_bl_len;   // BLOCK_LEN = 2^READ_BL_LEN
        mult = 1 << (c_size_mult + 2);  // MULT = 2^C_SIZE_MULT+2 (C_SIZE_MULT < 8)
        blocknr = (c_size + 1) * mult;  // BLOCKNR = (C_SIZE+1) * MULT
        capacity = (uint64_t)blocknr * block_len;  // memory capacity = BLOCKNR * BLOCK_LEN
        blocks = capacity / _block_size;
        /* DBG_PRINTF("Standard Capacity: c_size: %" PRIu32 "\r\n", c_size); */
        /* DBG_PRINTF("Sectors: 0x%llx : %llu\r\n", blocks, blocks); */
        /* DBG_PRINTF("Capacity: 0x%llx : %llu MB\r\n", capacity, (capacity / (1024U * 1024U))); */
        break;

    case 1:
        hc_c_size = ext_bits(csd, 69, 48);    // device size : C_SIZE : [69:48]
        blocks = (hc_c_size + 1) << 10;       // block count = C_SIZE+1) * 1K
        // byte (512B is block size)
        /* DBG_PRINTF("SDHC/SDXC Card: hc_c_size: %" PRIu32 "\r\n", hc_c_size); */
        /* DBG_PRINTF("Sectors: %8llu\r\n", blocks); */
        /* DBG_PRINTF("Capacity: %8llu MB\r\n", (blocks / (2048U))); */
        break;

    default:
        /* DBG_PRINTF("CSD struct unsupported\r\n"); */
        /* myASSERT(!"CSD struct unsupported\r\n"); */
        return 0;
    };
    return blocks;
}

uint64_t sd_sectors(sd_card_t *pSD) {
    sd_spi_select(pSD);
    uint64_t sectors = sd_sectors_nolock(pSD);
    sd_spi_deselect(pSD);
    return sectors;
}

sd_card_t* sd_create(master_spi_t* m_spi, uint8_t gpio_CS) {
    sd_card_t* sd = (sd_card_t*) malloc(sizeof(sd_card_t));
    sd->m_spi = m_spi;
    sd->ss_gpio = gpio_CS;
    sd->sectors = 0;
    sd->card_type = SDCARD_NONE;
    sd->pcName = NULL;
    sd->fatfs.fs_type = 0;
    sd->status = STA_NOINIT;

    sd->spi_slave_id = master_spi_add_slave(sd->m_spi, gpio_CS, SDCARD_MODE, SDCARD_BAUD);
    sd_spi_select(sd);

    int err = sd_init_medium(sd);
    if (SD_BLOCK_DEVICE_ERROR_NONE != err) {
        /* DBG_PRINTF("Failed to initialize card\r\n"); */
        sd_spi_deselect(sd);
        return sd;
    }
    /* DBG_PRINTF("SD card initialized\r\n"); */
    sd->sectors = sd_sectors_nolock(sd);
    if (0 == sd->sectors) {
        // CMD9 failed
        sd_spi_deselect(sd);
        return sd;
    }
    // Set block length to 512 (CMD16)
    if (sd_cmd(sd, CMD16_SET_BLOCKLEN, _block_size, false, 0) != 0) {
        /* DBG_PRINTF("Set %" PRIu32 "-byte block timed out\r\n", _block_size); */
        sd_spi_deselect(sd);
        return sd;
    }

    // The card is now initialized
    sd->status &= ~STA_NOINIT;

    // Set SCK for data transfer
    master_spi_set_baud(sd->m_spi, sd->spi_slave_id);
    sd_spi_deselect(sd);
    return sd;
}

void sd_free(sd_card_t* sd) {
    free(sd);
}

void sd_print(sd_card_t* sd) {
    printf("\nSD CARD:");
    printf("\n- spi_slave_id: %d", sd->spi_slave_id);
    printf("\n- ss_gpio: %d", sd->ss_gpio);
    printf("\n- sectors: %llu", sd->sectors);
    printf("\n- card_type: %d", sd->card_type);
    printf("\n- pcName: %s", sd->pcName);
    printf("\n- status: %d", sd->status);
    printf("\n- fatfs.fs_type: %d", sd->fatfs.fs_type);
    puts("\n");
}
