#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hw_model.h"
#include "data_model.h"
#include "input_processor.h"
#include "screen_processor.h"
#include "usb_hid.h"

void load_flash() {
    // load FLASH HEADER
    memset(kbd_system.flash_header, 0, KBD_FLASH_HEADER_SIZE);
    flash_read(kbd_hw.flash, 0, kbd_system.flash_header, KBD_FLASH_HEADER_SIZE);

    // load FLASH DATASETS
    init_flash_datasets(kbd_system.flash_datasets);

    for(uint8_t i=0; i<KBD_CONFIG_SCREEN_COUNT; i++)
        config_screen_default_initiators[i]();

    load_flash_datasets(kbd_system.flash_datasets);
}

void rtc_task(void* param) {
    (void)param;

    rtc_read_time(kbd_hw.rtc, &kbd_system.date);
    kbd_system.temperature = rtc_get_temperature(kbd_hw.rtc);
}

void apply_configs() {
    static bool init=true;
    static int16_t applied[KBD_CONFIG_SCREEN_COUNT];

    for(uint8_t i=0; i<KBD_CONFIG_SCREEN_COUNT; i++) {
        flash_dataset_t* fd = kbd_system.flash_datasets[i];
        if(applied[i]==fd->pos && !init) continue;

        config_screen_appliers[i]();
        applied[i] = fd->pos;
    }

    init = false;
}

void lcd_display_head_task(void* param) {
    (void)param;

    lcd_update_backlight(kbd_system.backlight);

    static uint8_t old_state[10];
    uint8_t state[10] = {
        kbd_system.screen & KBD_CONFIG_SCREEN_MASK ? 1 : 0,
        kbd_system.temperature,
        kbd_system.date.hour,
        kbd_system.date.minute,
        kbd_system.date.weekday%8,
        kbd_system.state.caps_lock ? 1 : 0,
        kbd_system.state.num_lock ? 1 : 0,
        kbd_system.state.scroll_lock ? 1 : 0,
        kbd_system.date.month,
        kbd_system.date.date
    };

    if(memcmp(old_state, state, 10)==0) return; // no change
    memcpy(old_state, state, 10);

    char txt[8];
    uint16_t bg = state[0] ? GRAY : BLACK;
    uint16_t fgm = RED;
    uint16_t fg = WHITE;
    uint16_t fg1 = YELLOW;
    uint16_t bg2 = DARK_GRAY;
    uint16_t fgcl = RED;
    uint16_t fgnl = GREEN;
    uint16_t fgsl = BLUE;

    // 2 rows 240x(20+20)
    static lcd_canvas_t* cv = NULL;
    if(!cv) cv = lcd_new_canvas(240, 40, bg);
    lcd_canvas_rect(cv, 0, 0, 240, 40, bg, 1, true);

    // row1: version, temperature, time, weekday
    // version : w:11x2=22 h:16, x:5-27
    char* vs[16] = {
        "0", "1", "2", "3", "4", "5", "6", "7",
        "8", "9", "A", "B", "C", "D", "E", "F",
    };
    lcd_canvas_text(cv, 5, 4, vs[0x0F & (kbd_system.version>>4)], &lcd_font16,
                    kbd_system.role==kbd_role_MASTER ? fgm : fg, bg);
    lcd_canvas_text(cv, 16, 4, vs[0x0F & kbd_system.version], &lcd_font16,
                    kbd_system.role==kbd_role_MASTER ? fg : fgm, bg);
    // temperature: w:11x2=22 h:16, x:40-62
    sprintf(txt, "%02d", state[1]);
    lcd_canvas_text(cv, 40, 4, txt, &lcd_font16, fg, bg);
    // time (2 rows) : w:17x5=85 h:24, x:77-162, y:8
    sprintf(txt, "%02d:%02d", state[2], state[3]);
    lcd_canvas_text(cv, 80, 9, txt, &lcd_font24, fg1, bg);
    // weekday: w:11x3=33 h:16, x:197-230
    char* wd = rtc_week[state[4]];
    lcd_canvas_text(cv, 197, 4, wd, &lcd_font16, fg, bg);

    // row2: locksx2, temperature, MM/dd
    // locksx3: w:20x2=40 h:16, x:5-65
    lcd_canvas_rect(cv, 5, 22, 20, 16, bg2, 1, true);
    if(state[5])
        lcd_canvas_circle(cv, 15, 30, 3, fgcl, 1, true);
    lcd_canvas_rect(cv, 25, 22, 20, 16, bg2, 1, true);
    if(state[6])
        lcd_canvas_circle(cv, 35, 30, 3, fgnl, 1, true);
    lcd_canvas_rect(cv, 45, 22, 20, 16, bg2, 1, true);
    if(state[7])
        lcd_canvas_circle(cv, 55, 30, 3, fgsl, 1, true);
    // MM/dd: w:11x5=55 h:16, x:180-235
    sprintf(txt, "%02d/%02d", state[8], state[9]);
    lcd_canvas_text(cv, 180, 22, txt, &lcd_font16, fg, bg);

    lcd_display_canvas(kbd_hw.lcd, 0, 0, cv);
}

void led_pixel_task(void* param) {
    (void)param;
    // TODO: prepare the colors buffers
    led_pixel_set(kbd_hw.led_pixel, kbd_system.led_colors_left, kbd_system.led_colors_right);
}


void tb_scan_task_capture(void* param) {
    kbd_tb_motion_t* ds = (kbd_tb_motion_t*) param;

    // scan tb scroll and write to sb_right_tb_motion
    bool has_motion = false;
    bool on_surface = false;
    int16_t dx = 0;
    int16_t dy = 0;

    has_motion = tb_check_motion(kbd_hw.tb, &on_surface, &dx, &dy);

    ds->has_motion = ds->has_motion || has_motion;
    ds->on_surface = ds->on_surface || on_surface;
    ds->dx = ds->dx + dx;
    ds->dy = ds->dy + dy;
}

void tb_scan_task_publish(void* param) {
    kbd_tb_motion_t* ds = (kbd_tb_motion_t*) param;

    write_shared_buffer(kbd_system.sb_right_tb_motion, time_us_64(), ds);

    ds->has_motion = false;
    ds->on_surface = false;
    ds->dx = 0;
    ds->dy = 0;
}

kbd_event_t process_basic_event(kbd_event_t event) {
    switch(event) {
    case kbd_backlight_event_HIGH:
        kbd_system.backlight = kbd_system.backlight <= 90 ? kbd_system.backlight+10 : 100;
        return kbd_event_NONE;
    case kbd_backlight_event_LOW:
        kbd_system.backlight = kbd_system.backlight >= 10 ? kbd_system.backlight-10 : 0;
        return kbd_event_NONE;
    default:
        return event;
    }
}

void process_idle() {
    static bool idle_prev = false;
    static uint8_t backlight_prev = 0;
    // 0xFF means never idle
    uint8_t mins = kbd_system.idle_minutes==0xFF ? 0 : (time_us_64()-kbd_system.active_ts)/60000000u;
    bool idle = mins >= kbd_system.idle_minutes;
    if(idle & !idle_prev) { // turned idle
        backlight_prev = kbd_system.backlight;
        kbd_system.backlight = 0;
    } else if(idle_prev & !idle) { // turned active
        kbd_system.backlight = backlight_prev;
    }
    idle_prev = idle;
}

/*
 * Processing:
 * sb_left_key_press      -->  sb_state
 * sb_right_key_press          sb_left_task_request
 * sb_right_tb_motion          sb_right_task_request
 * sb_left_task_response       hid_report_out
 * sb_right_task_response
 * hid_report_in
 */

void process_inputs(void* param) {
    (void)param;

    kbd_state_t old_state = kbd_system.state;

    // use the hid_report_in
    kbd_system.state.caps_lock = kbd_system.hid_report_in.keyboard.CapsLock;
    kbd_system.state.num_lock = kbd_system.hid_report_in.keyboard.NumLock;
    kbd_system.state.scroll_lock = kbd_system.hid_report_in.keyboard.ScrollLock;

    // read the task responses
    if(kbd_system.side==kbd_side_RIGHT) {
        if(kbd_system.left_task_response_ts != kbd_system.sb_left_task_response->ts)
            read_shared_buffer(kbd_system.sb_left_task_response,
                               &kbd_system.left_task_response_ts, kbd_system.left_task_response);
    } else {
        if(kbd_system.right_task_response_ts != kbd_system.sb_right_task_response->ts)
            read_shared_buffer(kbd_system.sb_right_task_response,
                               &kbd_system.right_task_response_ts, kbd_system.right_task_response);
    }

    // read the inputs
    uint64_t ts;
    read_shared_buffer(kbd_system.sb_left_key_press, &ts, kbd_system.left_key_press);
    read_shared_buffer(kbd_system.sb_right_key_press, &ts, kbd_system.right_key_press);
    read_shared_buffer(kbd_system.sb_right_tb_motion, &ts, &kbd_system.right_tb_motion);
    static uint64_t last_ts_tb_motion = 0;
    if(last_ts_tb_motion == ts) { 
        // discard the delta if already used
        kbd_system.right_tb_motion.dx = 0;
        kbd_system.right_tb_motion.dy = 0;
    } else {
        last_ts_tb_motion = ts;
    }

    // process inputs to update the hid_report_out and generate event
    kbd_event_t event = execute_input_processor();

    // track activity
    if(event!=kbd_event_NONE || kbd_system.hid_report_out.has_events) kbd_system.active_ts = time_us_64();

    // handle basic event
    event = process_basic_event(event);

    // send event to screens processor to process the responses and raise requests if any
    execute_screen_processor(event);

    // set the task requests
    if(kbd_system.side==kbd_side_RIGHT) {
        if(kbd_system.left_task_request_ts != kbd_system.sb_left_task_request->ts)
            write_shared_buffer(kbd_system.sb_left_task_request,
                                kbd_system.left_task_request_ts, kbd_system.left_task_request);
    } else {
        if(kbd_system.right_task_request_ts != kbd_system.sb_right_task_request->ts)
            write_shared_buffer(kbd_system.sb_right_task_request,
                                kbd_system.right_task_request_ts, kbd_system.right_task_request);
    }

    // note other state changes
    kbd_system.state.screen = kbd_system.screen;
    kbd_system.state.usb_hid_state = kbd_system.usb_hid_state;
    kbd_system.state.backlight = kbd_system.backlight;

    // update the state if changed
    if(memcmp(&old_state, &kbd_system.state, sizeof(kbd_state_t)) != 0) {
        kbd_system.state_ts = time_us_64();
        write_shared_buffer(kbd_system.sb_state, kbd_system.state_ts, &kbd_system.state);
    }

    usb_hid_task();
}

void process_requests() {
    // read the task request
    if(kbd_system.role == kbd_role_SLAVE) {
        if(kbd_system.side == kbd_side_LEFT) {
            if(kbd_system.left_task_request_ts != kbd_system.sb_left_task_request->ts)
                read_shared_buffer(kbd_system.sb_left_task_request,
                                   &kbd_system.left_task_request_ts, kbd_system.left_task_request);
        } else { // RIGHT
            if(kbd_system.right_task_request_ts != kbd_system.sb_right_task_request->ts)
                read_shared_buffer(kbd_system.sb_right_task_request,
                                   &kbd_system.right_task_request_ts, kbd_system.right_task_request);
        }
    }

    // respond to the reqeust
    respond_screen_processor();

    // set the task responses
    if(kbd_system.role == kbd_role_SLAVE) {
        if(kbd_system.side == kbd_side_LEFT) {
            if(kbd_system.left_task_response_ts != kbd_system.sb_left_task_response->ts)
                write_shared_buffer(kbd_system.sb_left_task_response,
                                    kbd_system.left_task_response_ts, kbd_system.left_task_response);
        } else { // RIGHT
            if(kbd_system.right_task_response_ts != kbd_system.sb_right_task_response->ts)
                write_shared_buffer(kbd_system.sb_right_task_response,
                                    kbd_system.right_task_response_ts, kbd_system.right_task_response);
        }
    }
}

void core0_main(void) {
    init_kbd_side();

    kbd_system.led = kbd_led_state_BLINK_HIGH;

    if(kbd_system.side == kbd_side_LEFT) init_hw_left();
    else init_hw_right();

    kbd_system.led = kbd_led_state_BLINK_FAST;

    // negotiate role with peer
    while((kbd_system.comm->role & 0xF0) != 0xF0) {
        usb_hid_task();

        if(kbd_system.usb_hid_state == kbd_usb_hid_state_MOUNTED && kbd_system.comm->peer_ready)
            peer_comm_try_master(kbd_system.comm, kbd_system.side==kbd_side_LEFT);
        tight_loop_contents();
    }

    kbd_system.led = kbd_led_state_BLINK_SLOW;

    // set role
    kbd_role_t role = kbd_system.comm->role == peer_comm_role_MASTER ? kbd_role_MASTER : kbd_role_SLAVE;
    set_kbd_role(role);

    // wait for core1
    while(!kbd_system.ready) {
        tight_loop_contents();
    }

    kbd_system.led = kbd_led_state_BLINK_NORMAL;

    uint32_t rtc_last_ms = 0;
    uint32_t lcd_last_ms = 0;
    uint32_t led_pixel_last_ms = 0;
    uint32_t tb_capture_last_ms = 0;
    uint32_t tb_publish_last_ms = 0;
    uint32_t proc_last_ms = 0;

    kbd_tb_motion_t tbm = {
        .has_motion = false,
        .on_surface = false,
        .dx = 0,
        .dy = 0
    };

    while(true) {
        // keep tiny usb ready
        usb_hid_idle_task();

        // get date & temperature, once a second
        do_if_elapsed(&rtc_last_ms, 1000, NULL, rtc_task);

        // apply any new configs
        apply_configs();

        if(kbd_system.side == kbd_side_LEFT) {
            // update lcd display head, @ 100 ms
            do_if_elapsed(&lcd_last_ms, 100, NULL, lcd_display_head_task);

            // set system led
            switch(kbd_system.usb_hid_state) {
            case kbd_usb_hid_state_UNMOUNTED:
                kbd_system.led = kbd_led_state_BLINK_FAST;
                break;
            case kbd_usb_hid_state_MOUNTED:
                kbd_system.led = kbd_led_state_BLINK_NORMAL;
                break;
            case kbd_usb_hid_state_SUSPENDED:
                kbd_system.led = kbd_led_state_BLINK_SLOW;
            }
        }
        else { // RIGHT
            // scan track ball scroll (capture), @ 3 ms
            do_if_elapsed(&tb_capture_last_ms, 3, &tbm, tb_scan_task_capture);

            // scan track ball scroll (publish), @ 20 ms
            do_if_elapsed(&tb_publish_last_ms, 20, &tbm, tb_scan_task_publish);

            // set caps lock led
            kbd_system.led = kbd_system.state.caps_lock ? kbd_led_state_ON : kbd_led_state_OFF;

            // set key switch leds
            do_if_elapsed(&led_pixel_last_ms, 1000, NULL, led_pixel_task);
        }

        if(kbd_system.role == kbd_role_MASTER) {
            // process input to output/usb, @ 10 ms
            do_if_elapsed(&proc_last_ms, 10, NULL, process_inputs);

            // handle idleness, only MASTER has activeness tracking
            process_idle();
        } else { // SLAVE
            if(kbd_system.state_ts != kbd_system.sb_state->ts)
                read_shared_buffer(kbd_system.sb_state, &kbd_system.state_ts, &kbd_system.state);

            kbd_system.backlight = kbd_system.state.backlight;
            kbd_system.usb_hid_state = kbd_system.state.usb_hid_state;
            kbd_system.screen = kbd_system.state.screen;
        }

        // execute on-demand tasks
        process_requests();

        tight_loop_contents();
    }
}
