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
 * - init hw (except left(lcd), right(tb) and core1(uart))
 * - detect side
 * - init hw left(lcd) or right(tb)
 * - init core1
 *   - init hw uart
 *   - init comm
 *   - loop
 *     - scan keys 6x7
 *     - init transfer cycle
 * - init usb
 *   - wait for usb or comm to be ready
 *     - setup role (master or slave)
 * - loop tasks
 *   - role master ->
 *     - process inputs (hid_report_in key_scan, tb_motion, responses)
 *       - update usb hid_report_out
 *       - update left/right request
 *       - update system_state
 *   - role slave ->
 *     - process requests
 *       - update responses
 *   - side left ->
 *     - update LCD
 *   - side right ->
 *     - read tb_motion
 */

void core1_main(void);

void core0_main(void);

void load_flash();

int main(void) {
    // stdio_init_all();

    init_data_model();

    init_hw_common();

    load_flash();

    usb_hid_init();

    kbd_system.led = kbd_led_state_BLINK_LOW;
    kbd_system.ledB = kbd_led_state_BLINK_LOW;

    multicore_launch_core1(core1_main);

    core0_main();
}
