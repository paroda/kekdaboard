/*
 * Abstraction over a set of shift registers chained together
 *
 * NOTE: NOT TESTED - EXPERIMENTAL
 */

#ifndef SHIFT_REGISTER_H
#define SHIFT_REGISTER_H

#include <stdlib.h>
#include "pico/stdlib.h"

#define CLOCK_DELAY_US 100000
#define LATCH_DELAY_US 100000

#define BIT_VALUE(i) (((uint) 1) << i)

typedef enum {
    SHIFT_REGISTER_MODE_SIPO,   // serial in parallel out
    SHIFT_REGISTER_MODE_PISO    // parallel in serial out
} shift_register_mode_t;

// Define shift register structure
typedef struct {
    shift_register_mode_t mode;

    uint latch_pin;    // Latch pin
    uint clock_pin;      // Clock pin
    uint data_pin;     // Data pin

    uint size;
    bool *bits;
} shift_register_t;

static shift_register_t * shift_register_new(shift_register_mode_t mode, uint size,
                                             uint latch_pin, uint clock_pin, uint data_pin) {
    // Allocate memory for new shift register object
    shift_register_t *sr = (shift_register_t *) malloc(sizeof (shift_register_t));

    // Set mode
    sr->mode = mode;

    // Set pins
    sr->clock_pin = clock_pin;
    sr->data_pin = data_pin;
    sr->latch_pin = latch_pin;

    // Allocate memory for bit data
    sr->size = size;
    sr->bits = (bool *) malloc(sr->size);

    // Set default pins state
    for(int i=0; i<sr->size; i++){
        sr->bits[i] = false;
    }

    // Return the created shift register
    return sr;
}

static void shift_register_free(shift_register_t *sr) {
    free(sr->bits);
    free(sr);
}

// transfer serial data to/from parallel bits
static void shift_register_sync(shift_register_t *sr) {
    if (sr->mode==SHIFT_REGISTER_MODE_PISO) { // for PISO pass parallel to serial
        // set latch down to move parallet bits to serial registers
        gpio_put(sr->latch_pin, true);
        sleep_us(LATCH_DELAY_US);

        // read serial registers

        for (int i = 0; i < sr->size; i++) {
            gpio_put(sr->clock_pin, true);
            sleep_us(CLOCK_DELAY_US);

            sr->bits[i] = gpio_get(sr->data_pin);

            gpio_put(sr->clock_pin, false);
            sleep_us(CLOCK_DELAY_US); // TODO: remove?
        }

        // pick latch up as reading is done
        gpio_put(sr->latch_pin, false);
        sleep_us(LATCH_DELAY_US); // TODO: remove?
    } else { // for SIPO pass serial to parallel
        // pick latch up to start wrting serial registers
        gpio_put(sr->latch_pin, true);
        sleep_us(LATCH_DELAY_US); // TODO remove?

        // write serial registers (in reverse order)
        for (int i = sr->size-1; i >= 0; i--) {
            gpio_put(sr->clock_pin, false);
            sleep_us(CLOCK_DELAY_US); // TODO remove?

            gpio_put(sr->data_pin, sr->bits[i]);

            gpio_put(sr->clock_pin, true);
            sleep_us(CLOCK_DELAY_US);
        }

        // set latch down to move from serial registers to parallel bits
        gpio_put(sr->latch_pin, false);
        sleep_us(LATCH_DELAY_US);
    }
}

#endif
