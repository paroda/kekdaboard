#include <stdio.h>
#include <string.h>

#include "hw_model.h"
#include "screen_processor.h"

#define CONFIG_VERSION 0x01
#define FIELD_COUNT 5

/*
 *   0123456789012  font
 * 1 .............  11x16 y:10
 * 2 CPI     01600  17x24 y:60
 * 3      Scale  Q        y:100
 * 4 Scroll 032  2        y:130
 * 5 Delta  004  2        y:180
 */

#define CPI_BASE 400
#define CPI_MULTIPLIER_MAX (KBD_TB_CPI_MAX/CPI_BASE)

typedef struct {
    uint8_t version;
    uint8_t cpi_multiplier; // cpi = CPI_BASE * cpi_multiplier, range 1-CPI_MULTIPLIER_MAX
    uint8_t scroll_scale; // 1-0xFF
    uint8_t scroll_quad_weight; // 0-9
    uint8_t delta_scale; // 1-0xFF
    uint8_t delta_quad_weight; // 0-9
} tb_motion_config_t;

static tb_motion_config_t tb_motion_config;
static flash_dataset_t* fd;

static uint8_t field;
static bool dirty;

static uint8_t select_next_value(uint8_t field) {
    switch(field) {
    case 0: // cpi_multiplier
        return tb_motion_config.cpi_multiplier = tb_motion_config.cpi_multiplier>=CPI_MULTIPLIER_MAX ?
            CPI_MULTIPLIER_MAX : tb_motion_config.cpi_multiplier+1;
    case 1: // scroll_scale
        return tb_motion_config.scroll_scale = tb_motion_config.scroll_scale>=240 ?
            255 : tb_motion_config.scroll_scale>=32 ?
            tb_motion_config.scroll_scale+16 : tb_motion_config.scroll_scale*2;
        break;
    case 2: // scroll_quad_weight
        return tb_motion_config.scroll_quad_weight = tb_motion_config.scroll_quad_weight>=9 ?
            9 : tb_motion_config.scroll_quad_weight+1;
        break;
    case 3: // delta_scale
        return tb_motion_config.delta_scale = tb_motion_config.delta_scale>=240 ?
            255 : tb_motion_config.delta_scale>=32 ?
            tb_motion_config.delta_scale+16 : tb_motion_config.delta_scale*2;
        break;
    case 4: // delta_quad_weight
        return tb_motion_config.delta_quad_weight = tb_motion_config.delta_quad_weight>=9 ?
            9 : tb_motion_config.delta_quad_weight+1;
        break;
    default: return 0; // invalid
    }
}

static uint8_t select_prev_value(uint8_t field) {
    switch(field) {
    case 0: // cpi_multiplier
        return tb_motion_config.cpi_multiplier = tb_motion_config.cpi_multiplier<=1 ?
            1 : tb_motion_config.cpi_multiplier-1;
    case 1: // scroll_scale
        return tb_motion_config.scroll_scale = tb_motion_config.scroll_scale<=1 ?
            1 : tb_motion_config.scroll_scale<=32 ?
            tb_motion_config.scroll_scale/2 : tb_motion_config.scroll_scale<=240 ?
            tb_motion_config.scroll_scale-16 : 240;
        break;
    case 2: // scroll_quad_weight
        return tb_motion_config.scroll_quad_weight = tb_motion_config.scroll_quad_weight<=0 ?
            0 : tb_motion_config.scroll_quad_weight-1;
        break;
    case 3: // delta_scale
        return tb_motion_config.delta_scale = tb_motion_config.delta_scale<=1 ?
            1 : tb_motion_config.delta_scale<=32 ?
            tb_motion_config.delta_scale/2 : tb_motion_config.delta_scale<=240 ?
            tb_motion_config.delta_scale-16 : 240;
        break;
    case 4: // delta_quad_weight
        return tb_motion_config.delta_quad_weight = tb_motion_config.delta_quad_weight<=0 ?
            0 : tb_motion_config.delta_quad_weight-1;
        break;
    default: return 0; // invalid
    }
}

static uint8_t set_field(uint8_t field, uint8_t value) {
    switch(field) {
    case 0: // cpi_multiplier
        if(value>=1 && value<=CPI_MULTIPLIER_MAX) tb_motion_config.cpi_multiplier = value;
        return tb_motion_config.cpi_multiplier;
    case 1: // scroll_scale
        if(value>=1) tb_motion_config.scroll_scale = value;
        return tb_motion_config.scroll_scale;
    case 2: // scroll_quad_weight
        if(value<=9) tb_motion_config.scroll_quad_weight = value;
        return tb_motion_config.scroll_quad_weight;
    case 3: // delta_scale
        if(value>=1) tb_motion_config.delta_scale = value;
        return tb_motion_config.delta_scale;
    case 4: // delta_quad_weight
        if(value<=9) tb_motion_config.delta_quad_weight = value;
        return tb_motion_config.delta_quad_weight;
    default: return 0; // invalid
    }
}

static void save() {
    memcpy(fd->data, &tb_motion_config, sizeof(tb_motion_config_t));
    flash_store_save(fd);
}

static void draw_cpi(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    sprintf(txt, "%6d", tb_motion_config.cpi_multiplier*CPI_BASE);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void draw_scroll_scale(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    sprintf(txt, "%3d", tb_motion_config.scroll_scale);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void draw_scroll_quad_weight(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    sprintf(txt, "%3d", tb_motion_config.scroll_quad_weight);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void draw_delta_scale(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    sprintf(txt, "%3d", tb_motion_config.delta_scale);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void draw_delta_quad_weight(lcd_canvas_t* cv, uint16_t x, uint16_t y, bool selected) {
    char txt[8];
    sprintf(txt, "%3d", tb_motion_config.delta_quad_weight);
    lcd_canvas_text(cv, x, y, txt, &lcd_font24, selected?RED:WHITE, LCD_BODY_BG);
}

static void init_screen() {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    char txt[16];
    sprintf(txt, "Trackball-%04d", fd->pos);
    lcd_canvas_text(cv, 43, 10, txt, &lcd_font16, BLUE, LCD_BODY_BG);

    lcd_canvas_text(cv, 10, 60, "CPI", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_cpi(cv, 129, 60, field==0);

    lcd_canvas_text(cv, 95, 100, "Scale  Q", &lcd_font24, DARK_GRAY, LCD_BODY_BG);

    lcd_canvas_text(cv, 10, 130, "Scroll", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_scroll_scale(cv, 129, 130, field==1);
    draw_scroll_quad_weight(cv, 180, 130, field==2);

    lcd_canvas_text(cv, 10, 160, "Delta", &lcd_font24, DARK_GRAY, LCD_BODY_BG);
    draw_delta_scale(cv, 129, 160, field==3);
    draw_delta_quad_weight(cv, 180, 160, field==4);

    lcd_display_body();
}

static void draw_field(lcd_canvas_t* cv1, lcd_canvas_t* cv2, uint8_t field, bool selected) {
    switch(field) {
    case 0: // cpi
        draw_cpi(cv2, 0, 0, selected);
        lcd_display_body_canvas(129, 60, cv2);
        break;
    case 1: // scroll_scale
        draw_scroll_scale(cv1, 0, 0, selected);
        lcd_display_body_canvas(129, 130, cv1);
        break;
    case 2: // scroll_quad_weight
        draw_scroll_quad_weight(cv1, 0, 0, selected);
        lcd_display_body_canvas(180, 130, cv1);
        break;
    case 3: // delta_scale
        draw_delta_scale(cv1, 0, 0, selected);
        lcd_display_body_canvas(129, 160, cv1);
        break;
    case 4: // delta_quad_weight
        draw_delta_quad_weight(cv1, 0, 0, selected);
        lcd_display_body_canvas(180, 160, cv1);
        break;
    default: break; // invalid
    }
}

static void update_screen(uint8_t field, uint8_t sel_field) {
    if(kbd_system.side != kbd_side_LEFT) return;

    lcd_canvas_t* cv1 = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 51, 24, LCD_BODY_BG);
    lcd_canvas_t* cv2 = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 102, 24, LCD_BODY_BG);

    if(field!=sel_field) {
        draw_field(cv1, cv2, field, false);
        lcd_canvas_clear(cv2); // clear the bigger one of the shared canvases
    }

    draw_field(cv1, cv2, sel_field, true);
    lcd_free_canvas(cv1);
    lcd_free_canvas(cv2);

    if(dirty) {
        lcd_canvas_t* cv = lcd_new_shared_canvas(kbd_hw.lcd_body->buf, 10, 10, LCD_BODY_BG);
        lcd_canvas_circle(cv, 5, 5, 5, RED, 1, true);
        lcd_display_body_canvas(220, 10, cv);
        lcd_free_canvas(cv);
    }
}

static void init_data() {
    tb_motion_config.version = CONFIG_VERSION;
    tb_motion_config.cpi_multiplier = kbd_system.tb_cpi / CPI_BASE;
    tb_motion_config.scroll_scale = kbd_system.tb_scroll_scale;
    tb_motion_config.scroll_quad_weight = kbd_system.tb_scroll_quad_weight;
    tb_motion_config.delta_scale = kbd_system.tb_delta_scale;
    tb_motion_config.delta_quad_weight = kbd_system.tb_delta_quad_weight;
}

void execute_screen_tb(kbd_event_t event) {
    uint8_t* lreq = kbd_system.left_task_request;
    uint8_t* rreq = kbd_system.right_task_request;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        init_data();
        mark_left_request(kbd_config_screen_tb);
        lreq[2] = 0;
        lreq[3] = field = 0;
        memcpy(lreq+4, &tb_motion_config, sizeof(tb_motion_config_t));
        dirty = false;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_UP:
    case kbd_screen_event_DOWN:
        mark_left_request(kbd_config_screen_tb);
        lreq[2] = 1;
        lreq[3] = field;
        switch(event) {
        case kbd_screen_event_LEFT:
            lreq[4] = field = field==0 ? FIELD_COUNT-1 : field-1;
            break;
        case kbd_screen_event_RIGHT:
            lreq[4] = field = field==FIELD_COUNT-1 ? 0 : field+1;
            break;
        case kbd_screen_event_UP:
            switch(field) {
            case 0: lreq[4] = field = 4; break;
            case 1: lreq[4] = field = 0; break;
            case 2: lreq[4] = field = 3; break;
            case 3: lreq[4] = field = 1; break;
            case 4: lreq[4] = field = 2; break;
            default: lreq[4] = field = 0; break;
            }
            break;
        case kbd_screen_event_DOWN:
            switch(field) {
            case 0: lreq[4] = field = 1; break;
            case 1: lreq[4] = field = 3; break;
            case 2: lreq[4] = field = 4; break;
            case 3: lreq[4] = field = 2; break;
            case 4: lreq[4] = field = 0; break;
            default: lreq[4] = field = 0; break;
            }
            break;
        default: lreq[4] = field = 0; break;
        }
        break;
    case kbd_screen_event_SEL_PREV:
    case kbd_screen_event_SEL_NEXT:
        mark_left_request(kbd_config_screen_tb);
        lreq[2] = 2;
        lreq[3] = field;
        lreq[4] = event==kbd_screen_event_SEL_PREV ? select_prev_value(field) : select_next_value(field);
        lreq[5] = dirty = true;
        break;
    case kbd_screen_event_SAVE:
        mark_left_request(kbd_config_screen_tb);
        mark_right_request(kbd_config_screen_tb);
        rreq[2] = lreq[2] = 3;
        rreq[3] = lreq[3] = field = 0;
        memcpy(lreq+4, &tb_motion_config, sizeof(tb_motion_config_t));
        memcpy(rreq+4, &tb_motion_config, sizeof(tb_motion_config_t));
        dirty = false;
        break;
    default: break;
    }
}

void respond_screen_tb() {
    uint8_t* req = kbd_system.side == kbd_side_LEFT ?
        kbd_system.left_task_request : kbd_system.right_task_request;
    switch(req[2]) {
    case 0: // init
    case 3: // save
        field = req[3];
        memcpy(&tb_motion_config, req+4, sizeof(tb_motion_config_t));
        if(req[2]==3) save();
        dirty = false;
        init_screen();
        break;
    case 1: // select field
        field = req[4];
        update_screen(req[3], req[4]);
        break;
    case 2: // set field
        field = req[3];
        set_field(req[3], req[4]);
        dirty = req[5];
        update_screen(req[3], req[3]);
        break;
    default: break;
    }

    if(kbd_system.side == kbd_side_LEFT)
        mark_left_response();
    else
        mark_right_response();
}

void init_config_screen_default_tb() {
    init_data();

    fd = kbd_system.flash_datasets[get_screen_index(kbd_config_screen_tb)];
    memset(fd->data, 0xFF, FLASH_DATASET_SIZE); // use 0xFF = erased state in flash
    memcpy(fd->data, &tb_motion_config, sizeof(tb_motion_config_t));
}

void apply_config_screen_tb() {
    if(fd->data[0]!=CONFIG_VERSION) return;

    memcpy(&tb_motion_config, fd->data, sizeof(tb_motion_config_t));

    kbd_system.tb_cpi = CPI_BASE * tb_motion_config.cpi_multiplier;
    kbd_system.tb_scroll_scale = tb_motion_config.scroll_scale;
    kbd_system.tb_scroll_quad_weight = tb_motion_config.scroll_quad_weight;
    kbd_system.tb_delta_scale = tb_motion_config.delta_scale;
    kbd_system.tb_delta_quad_weight = tb_motion_config.delta_quad_weight;

    if(kbd_system.side == kbd_side_RIGHT) {
        // must clearout the motion registers before setting the CPI
        int16_t dx,dy;
        bool on_surface;
        tb_check_motion(kbd_hw.tb, &on_surface, &dx, &dy); // make a dummy call to clear motions
        tb_set_cpi(kbd_hw.tb, kbd_system.tb_cpi);
    }
}
