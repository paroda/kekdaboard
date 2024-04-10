#include<stdlib.h>
#include<stdio.h>
#include<string.h>

#include "pico/stdlib.h"
#include "tb_pmw3389.h"
#include "srom_pmw3389.h"

// Registers
#define tb_Product_ID                0x00
#define tb_Revision_ID               0x01
#define tb_Motion                    0x02
#define tb_Delta_X_L                 0x03
#define tb_Delta_X_H                 0x04
#define tb_Delta_Y_L                 0x05
#define tb_Delta_Y_H                 0x06
#define tb_SQUAL                     0x07
#define tb_Raw_Data_Sum              0x08
#define tb_Maximum_Raw_data          0x09
#define tb_Minimum_Raw_data          0x0A
#define tb_Shutter_Lower             0x0B
#define tb_Shutter_Upper             0x0C
#define tb_Ripple_Control            0x0D
#define tb_Resolution_L              0x0E
#define tb_Resolution_H              0x0F
#define tb_Config2                   0x10
#define tb_Angle_Tune                0x11
#define tb_Frame_Capture             0x12
#define tb_SROM_Enable               0x13
#define tb_Run_Downshift             0x14
#define tb_Rest1_Rate_Lower          0x15
#define tb_Rest1_Rate_Upper          0x16
#define tb_Rest1_Downshift           0x17
#define tb_Rest2_Rate_Lower          0x18
#define tb_Rest2_Rate_Upper          0x19
#define tb_Rest2_Downshift           0x1A
#define tb_Rest3_Rate_Lower          0x1B
#define tb_Rest3_Rate_Upper          0x1C
#define tb_Observation               0x24
#define tb_Data_Out_Lower            0x25
#define tb_Data_Out_Upper            0x26
#define tb_SROM_ID                   0x2A
#define tb_Min_SQ_Run                0x2B
#define tb_Raw_Data_Threshold        0x2C
#define tb_Control2                  0x2D
#define tb_Config5_L                 0x2E
#define tb_Config5_H                 0x2F
#define tb_Power_Up_Reset            0x3A
#define tb_Shutdown                  0x3B
#define tb_Inverse_Product_ID        0x3F
#define tb_LiftCutoff_Cal3           0x41
#define tb_Angle_Snap                0x42
#define tb_LiftCutoff_Cal1           0x4A
#define tb_Motion_Burst              0x50
#define tb_SROM_Load_Burst           0x62
#define tb_Lift_Config               0x63
#define tb_Raw_Data_Burst            0x64
#define tb_LiftCutoff_Cal2           0x65
#define tb_LiftCutoff_Cal_Timeout    0x71
#define tb_LiftCutoff_Cal_Min_Length 0x72
#define tb_PWM_Period_Cnt            0x73
#define tb_PWM_Width_Cnt             0x74

#define GPIO_NONE 0xFF

static uint8_t tb_read_register(tb_t* tb, uint8_t reg_addr) {
    // book spi for this slave device
    master_spi_select_slave(tb->m_spi, tb->spi_slave_id);

    // send address of the register, with MSBit=0 to indicate it's a read
    reg_addr &= 0x7F;
    master_spi_write8(tb->m_spi, &reg_addr, 1);
    sleep_us(35); // tSRAD

    // read data
    uint8_t data;
    master_spi_read8(tb->m_spi, &data, 1);
    sleep_us(1); // tSCLK-NCS for read operation is 120ns

    // release spi
    master_spi_release_slave(tb->m_spi, tb->spi_slave_id);
    sleep_us(19); // tSRW/tSRR (=20us) minus tSCLK-NCS

    return data;
}

static void tb_write_register(tb_t* tb, uint8_t reg_addr, uint8_t data) {
    // book spi for this slave device
    master_spi_select_slave(tb->m_spi, tb->spi_slave_id);

    // send address of the register, with MSBit=1 to indicate it's a write
    // and the data as the next byte
    uint8_t buf[] = { reg_addr | 0x80, data };
    master_spi_write8(tb->m_spi, buf, 2);
    sleep_us(20); // tSCLK-NCS for write operation

    // release spi
    master_spi_release_slave(tb->m_spi, tb->spi_slave_id);
    sleep_us(100); // tSWW/tSWR (=120us) minus tSCLK-NCS. Could be shortened, but it looks like a safer lower bound
}

static void tb_upload_firmware(tb_t* tb) {
    // send the firmware to the chip
    uint8_t buf[2];

    // write 0 to Rest_En bit of Config2 register to disable Rest mode.
    tb_write_register(tb, tb_Config2, 0x00);

    // write 0x1d in SROM_enable reg for initializing
    tb_write_register(tb, tb_SROM_Enable, 0x1d);

    // wait for more than one frame period
    sleep_ms(10); // assume that the frame rate is as low as 100fps.. even if it should never be that low

    // write 0x18 to SROM_Enable to start SROM download
    tb_write_register(tb, tb_SROM_Enable, 0x18);

    // begin to write the SROM file (firmware_data)
    master_spi_select_slave(tb->m_spi, tb->spi_slave_id);

    // write burst dest address
    buf[0] = tb_SROM_Load_Burst|0x80;
    master_spi_write8(tb->m_spi, buf, 1);
    sleep_us(15);

    // send all bytes of the firmware
    for(int i=0; i<firmware_length; i++) {
        master_spi_write8(tb->m_spi, firmware_data+i, 1);
        sleep_us(15);
    }

    // read the SROM_ID register to verify the ID before any other register reads or writes
    tb_read_register(tb, tb_SROM_ID);

    // write 0x00 (rest disable) to Config2 register for wired mouse or 0x20 for wireless mouse design.
    tb_write_register(tb, tb_Config2, 0x00);

    // end writing SROM
    master_spi_release_slave(tb->m_spi, tb->spi_slave_id);

}

void tb_set_cpi(tb_t* tb, uint16_t cpi) {
    cpi = cpi>KBD_TB_CPI_MAX ? KBD_TB_CPI_MAX : cpi<KBD_TB_CPI_MIN ? KBD_TB_CPI_MIN : cpi;
    master_spi_set_baud(tb->m_spi, tb->spi_slave_id);

    uint16_t cpival = cpi/50; // CPI is set as multiples of 50
    tb->cpi = cpival*50;

    master_spi_select_slave(tb->m_spi, tb->spi_slave_id);
    tb_write_register(tb, tb_Resolution_L, cpival & 0xFF);
    tb_write_register(tb, tb_Resolution_H, (cpival >> 8) & 0xFF);
    master_spi_release_slave(tb->m_spi, tb->spi_slave_id);
}

void tb_device_signature(tb_t* tb,
                         uint8_t* product_id, uint8_t* inverse_product_id,
                         uint8_t* srom_version, uint8_t* motion) {
    master_spi_set_baud(tb->m_spi, tb->spi_slave_id);

    uint8_t reg[4] = { 0x00, 0x3F, 0x2A, 0x02 };
    uint8_t res[4];
    /* char* oregname[] = { "Product_ID", "Inverse_Product_ID", "SROM_Version", "Motion" }; */
    for(int i=0; i<4; i++) {
        res[i] = tb_read_register(tb, reg[i]);
    }
    *product_id = res[0];
    *inverse_product_id = res[1];
    *srom_version = res[2];
    *motion = res[3];
}

static void tb_connect_device(tb_t* tb, uint8_t gpio_CS,
                              uint8_t gpio_MT, uint8_t gpio_RST) {

    // register the track-ball as a slave with master spi, MODE 3, 4 MHz
    /// Only 4 MHz works, shouldn't be any higher or lower.
    tb->spi_slave_id = master_spi_add_slave(tb->m_spi, gpio_CS, 3, 4 * 1000 * 1000);
    master_spi_set_baud(tb->m_spi, tb->spi_slave_id);

    // initialize MT pin
    tb->gpio_MT = gpio_MT;
    if(tb->gpio_MT != GPIO_NONE) {
        gpio_init(tb->gpio_MT);
        gpio_set_dir(tb->gpio_MT, GPIO_IN);
    }

    // initialize RST pin when not GPIO_NONE
    tb->gpio_RST = gpio_RST;
    if(tb->gpio_RST != GPIO_NONE) {
        gpio_init(tb->gpio_RST);
        gpio_set_dir(tb->gpio_RST, GPIO_OUT);
        gpio_put(tb->gpio_RST, true);
    }
}

void tb_reset_device(tb_t* tb, uint16_t cpi) {
    // shutdown first
    tb_write_register(tb, tb_Shutdown, 0xb6);
    sleep_ms(300);

    // drop and raise ncs to reset spi port
    master_spi_select_slave(tb->m_spi, tb->spi_slave_id);
    sleep_us(40);
    master_spi_release_slave(tb->m_spi, tb->spi_slave_id);
    sleep_us(40);

    // force reset
    tb_write_register(tb, tb_Power_Up_Reset, 0x5a);
    sleep_ms(50); // wait for it to reboot

    // read registers 0x02 to 0x06 (and discard the data)
    tb_read_register(tb, tb_Motion);
    tb_read_register(tb, tb_Delta_X_L);
    tb_read_register(tb, tb_Delta_X_H);
    tb_read_register(tb, tb_Delta_Y_L);
    tb_read_register(tb, tb_Delta_Y_H);

    // upload the firmware
    tb_upload_firmware(tb);
    sleep_ms(10);

    // set CPI resolution
    tb_set_cpi(tb, cpi);
}

tb_t* tb_create(master_spi_t* m_spi,
                uint8_t gpio_CS, uint8_t gpio_MT, uint8_t gpio_RST,
                uint16_t cpi,
                bool swap_XY, bool invert_X, bool invert_Y) {
    tb_t* tb = (tb_t*) malloc(sizeof(tb_t));
    tb->m_spi = m_spi;

    // set the X/Y axis orientation
    tb->swap_XY = swap_XY;
    tb->invert_X = invert_X;
    tb->invert_Y = invert_Y;

    // Prepare the SPI port
    tb_connect_device(tb, gpio_CS, gpio_MT, gpio_RST);

    // Prepare the chip
    tb_reset_device(tb, cpi);

    return tb;
}

void tb_free(tb_t* tb) {
    free(tb);
}

bool tb_check_motion(tb_t* tb, bool* on_surface, int16_t* dx, int16_t* dy) {
    master_spi_set_baud(tb->m_spi, tb->spi_slave_id);

    uint8_t reg;
    uint8_t buf[12];

    tb_write_register(tb, tb_Motion_Burst, 0x00);

    master_spi_select_slave(tb->m_spi, tb->spi_slave_id);

    reg = tb_Motion_Burst;
    master_spi_write8(tb->m_spi, &reg, 1);
    sleep_us(35); // wait tSRAD
    master_spi_read8(tb->m_spi, buf, 12);
    sleep_us(1); // tSCLK-NCS for read operation is 120ns

    master_spi_release_slave(tb->m_spi, tb->spi_slave_id);

    /*
      BYTE[00] = Motion    = if the 7th bit is 1, a motion is detected.
      ==> 7 bit: MOT (1 when motion is detected)
      ==> 3 bit: 0 when chip is on surface / 1 when off surface
      ] = Observation
      BYTE[02] = Delta_X_L = dx (LSB)
      BYTE[03] = Delta_X_H = dx (MSB)
      BYTE[04] = Delta_Y_L = dy (LSB)
      BYTE[05] = Delta_Y_H = dy (MSB)
      BYTE[06] = SQUAL     = Surface Quality register, max 0x80
      - Number of features on the surface = SQUAL * 8
      BYTE[07] = Raw_Data_Sum   = It reports the upper byte of an 18‐bit counter which sums all 1296 raw data in the current frame;
      * Avg value = Raw_Data_Sum * 1024 / 1296
      BYTE[08] = Maximum_Raw_Data  = Max raw data value in current frame, max=127
      BYTE[09] = Minimum_Raw_Data  = Min raw data value in current frame, max=127
      BYTE[10] = Shutter_Upper     = Shutter LSB
      BYTE[11] = Shutter_Lower     = Shutter MSB, Shutter = shutter is adjusted to keep the average raw data values within normal operating ranges
    */

    bool has_motion = (buf[0] & 0x80) > 0;
    *on_surface = (buf[0] & 0x08) == 0; // 0 if on surface / 1 if off surface

    int xl = buf[2];
    int xh = buf[3];
    int yl = buf[4];
    int yh = buf[5];

    // int squal = buf[6];

    int x = xh<<8 | xl;
    int y = yh<<8 | yl;

    if(tb->swap_XY) {
        *dx = y;
        *dy = x;
    } else {
        *dx = x;
        *dy = y;
    }

    if(tb->invert_X) *dx = -*dx;
    if(tb->invert_Y) *dy = -*dy;

    return has_motion;
}

void print_tb(tb_t* tb) {
    printf("\n\nTrack ball:");
    printf("\ntb->spi_slave_id %d", tb->spi_slave_id);
    printf("\ntb->gpio_MT %d", tb->gpio_MT);
    printf("\ntb->gpio_RST %d", tb->gpio_RST);
    printf("\ntb->cpi %d", tb->cpi);
    printf("\ntb->swap_XY %d", tb->swap_XY);
    printf("\ntb->invert_X %d", tb->invert_X);
    printf("\ntb->invert_Y %d", tb->invert_Y);

    uint8_t product_id=0, inverse_product_id=0, srom_version=0, motion=0;
    tb_device_signature(tb, &product_id, &inverse_product_id, &srom_version, &motion);
    printf("\ntb device: product_id %d, inv product_id %d, srom v %d, motion %d",
           product_id, inverse_product_id, srom_version, motion);
}
