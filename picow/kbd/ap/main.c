#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/multicore.h"

#include "hw_model.h"
#include "data_model.h"

#include "usb_hid.h"

/*
 * Process Overview
 *
 * - init data_model
 * - init hw
 * - init core1
 *   - init hw wifi
 *     - UDP: receive key_scan and tb_motion, reply state, led_pixels
 *     - TCP: receive program and flash self
 *   - loop
 *     - wifi poll
 *     - animate led pixels
 * - init usb
 * - loop tasks
 *   - process inputs (hid_report_in key_scan, tb_motion, responses)
 *   - update usb hid_report_out
 *   - update left/right request
 *   - update system_state
 */

void core1_main(void);

void core0_main(void);

void load_flash();

int main(void) {

}
