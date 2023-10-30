#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw_model.h"
#include "data_model.h"

static void toggle_led(void* param) {
    kbd_led_t* led = (kbd_led_t*) param;
    led->on = !led->on;
    gpio_put(led->gpio, led->on);
}

static int32_t led_millis_to_toggle(kbd_led_t* led, kbd_led_state_t state) {
    // return milliseconds after which to toggle the led
    // return -1 to indicate skip toggle
    switch(state) {
    case kbd_led_state_OFF:
        return led->on ? 0 : -1;
    case kbd_led_state_ON:
        return led->on ? -1 : 0;
    case kbd_led_state_BLINK_LOW:    // 100(on), 900(off)
        return led->on ? 100 : 900;
    case kbd_led_state_BLINK_HIGH:   // 900, 100
        return led->on ? 900 : 100;
    case kbd_led_state_BLINK_SLOW:   // 1000, 1000
        return led->on ? 2000 : 2000;
    case kbd_led_state_BLINK_NORMAL: // 500, 500
        return led->on ? 500 : 500;
    case kbd_led_state_BLINK_FAST:   // 100, 100
        return led->on ? 50 : 50;
    }
    return -1;
}

static void led_task() {
    static uint32_t led_last_ms = 0;
    int32_t led_ms = led_millis_to_toggle(&kbd_hw.led, kbd_system.led);
    if(led_ms>=0) do_if_elapsed(&led_last_ms, led_ms, &kbd_hw.led, toggle_led);

    static uint32_t ledB_last_ms = 0;
    int32_t ledB_ms = led_millis_to_toggle(&kbd_hw.ledB, kbd_system.ledB);
    if(ledB_ms>=0) do_if_elapsed(&ledB_last_ms, ledB_ms, &kbd_hw.ledB, toggle_led);
}

static void key_scan_task(void* param) {
    (void)param;
    // scan key presses with kbd_hw.ks and save to sb_(left|right)_key_press
    shared_buffer_t* sb = (kbd_system.side == kbd_side_LEFT) ?
        kbd_system.sb_left_key_press : kbd_system.sb_right_key_press;
    key_scan_update(kbd_hw.ks);
    write_shared_buffer(sb, time_us_64(), kbd_hw.ks->keys);
}

static void comm_task(void* param) {
    (void)param;
    peer_comm_init_cycle(kbd_system.comm);
}

void core1_main(void) {
    init_hw_core1(kbd_system.comm);

    kbd_system.ledB = kbd_led_state_BLINK_HIGH;

    // connect to peer
    while(!kbd_system.comm->peer_ready) {
        led_task();
        peer_comm_try_peer(kbd_system.comm);
        tight_loop_contents();
    }

    kbd_system.ledB = kbd_led_state_BLINK_FAST;

    // wait for core0 to set role
    while(kbd_system.role==kbd_role_NONE) {
        led_task();
        tight_loop_contents();
    }

    while(!kbd_system.comm->peer_version) {
        led_task();
        if(kbd_system.role==kbd_role_MASTER) peer_comm_try_version(kbd_system.comm);
        tight_loop_contents();
    }

    // note the combined version
    kbd_system.version = kbd_system.side == kbd_side_LEFT ?
        (kbd_system.comm->version << 4) | kbd_system.comm->peer_version :
        (kbd_system.comm->peer_version << 4) | kbd_system.comm->version;

    kbd_system.ready = true;

    kbd_system.ledB = kbd_led_state_BLINK_NORMAL;

    uint32_t ks_last_ms = 0;
    uint32_t comm_last_ms = 0;

    while(true) {
        led_task();

        // scan key presses, @ 3 ms
        do_if_elapsed(&ks_last_ms, 3, NULL, key_scan_task);

        // transfer data to/from peer, initiated by master only, @ 3 ms
        if(kbd_system.role == kbd_role_MASTER) {
            do_if_elapsed(&comm_last_ms, 3, NULL, comm_task);
        }

        tight_loop_contents();
    }
}
