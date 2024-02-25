#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/multicore.h"

#include "hw_model.h"
#include "data_model.h"

/*
 * Process Overview
 *
 * - init data_model
 * - init hw
 * - init core1
 *   - init hw wifi
 *     - UDP: receive state, led_pixels
 *     - TCP: receive program and flash self
 *   - loop
 *     - wifi poll
 *     - scan keys 6x7
 *     - UDP: send key_scan
 *     - update leds
 * - loop tasks
 *   - process requests
 *     - update responses
 *   - update LCD
 */

void core1_main(void);

void core0_main(void);

int main(void) {

}
