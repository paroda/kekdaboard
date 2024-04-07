#include <stdio.h>
#include <string.h>

#include "../hw_model.h"
#include "../data_model.h"

#include "../util/tb_pmw3389.h"

#define THIS_SCREEN kbd_config_screen_tb
#define CONFIG_VERSION 0x01

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

#ifdef KBD_NODE_AP

static flash_dataset_t* fd;

void handle_screen_event_tb(kbd_event_t event) {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* lreq = c->left_task_request;
    uint8_t* rreq = c->right_task_request;
    uint8_t* lres = c->left_task_response;

    if(is_nav_event(event)) return;

    switch(event) {
    case kbd_screen_event_INIT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        init_task_request(rreq, &c->right_task_request_ts, THIS_SCREEN);
        lreq[2] = rreq[2] = 1;
        lreq[3] = fd->pos;
        memcpy(lreq+4, &tb_motion_config, sizeof(tb_motion_config_t));
        break;
    case kbd_screen_event_SAVE:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 2;
        break;
    case kbd_screen_event_LEFT:
    case kbd_screen_event_RIGHT:
    case kbd_screen_event_UP:
    case kbd_screen_event_DOWN:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 3;
        lreq[3] = 1;
        lreq[4] = event;
        break;
    case kbd_screen_event_SEL_PREV:
    case kbd_screen_event_SEL_NEXT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 4;
        lreq[3] = 1;
        lreq[4] = event;
        break;
    case kbd_screen_event_RESPONSE:
        if(lres[0] && lres[1]==THIS_SCREEN && lres[2]==1) {
            // save to flash
            memcpy(&tb_motion_config, lres+4, sizeof(tb_motion_config_t));
            memcpy(fd->data, &tb_motion_config, sizeof(tb_motion_config_t));
            flash_store_save(fd);
            // show on lcd
            init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
            lreq[2] = 1;
            lreq[3] = fd->pos;
            memcpy(lreq+4, fd->data, sizeof(tb_motion_config_t));
        }
        break;
    default: break;
    }
}

#endif

#ifdef KBD_NODE_LEFT

#define FIELD_COUNT 5

static uint8_t fd_pos;
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
    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    char txt[16];
    sprintf(txt, "Trackball-%04d", fd_pos);
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

void work_screen_task_tb() {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* req = c->task_request;
    uint8_t* res = c->task_response;

    uint8_t up_fields[FIELD_COUNT] = {4,0,3,1,2};
    uint8_t down_fields[FIELD_COUNT] = {1,3,4,2,0};
    uint8_t old_field;
    switch(req[2]) {
    case 1: // init
        fd_pos = req[3];
        memcpy(&tb_motion_config, req+4, sizeof(tb_motion_config_t));
        field = 0; dirty = false;
        init_screen();
        break;
    case 2: // save
        res[2] = 1;
        res[3] = fd_pos;
        memcpy(res+4, &tb_motion_config, sizeof(tb_motion_config_t));
        break;
    case 3: // select field
        old_field = field;
        switch(req[4]) {
        case kbd_screen_event_LEFT:
            field = field==0 ? FIELD_COUNT-1 : field-1;
            break;
        case kbd_screen_event_RIGHT:
            field = field==FIELD_COUNT-1 ? 0 : field+1;
            break;
        case kbd_screen_event_UP:
            field = up_fields[field];
            break;
        case kbd_screen_event_DOWN:
            field = down_fields[field];
            break;
        default: break;
        }
        update_screen(old_field, field);
        break;
    case 4: // select value
        set_field(field, req[4]==kbd_screen_event_SEL_PREV
                  ? select_prev_value(field)
                  : select_next_value(field));
        dirty = true;
        update_screen(field, field);
        break;
    default: break;
    }
}

#endif

#ifdef KBD_NODE_RIGHT

void work_screen_task_tb() {} // no action

#endif

void init_config_screen_data_tb() {
    uint8_t si = get_screen_index(THIS_SCREEN);
#ifdef KBD_NODE_AP
    fd = kbd_system.core1.flash_datasets[si];
    uint8_t* data = fd->data;
#else
    uint8_t* data = kbd_system.core1.flash_data[si];
#endif

    memset(data, 0xFF, KBD_TASK_DATA_SIZE); // use 0xFF = erased state in flash

    kbd_tb_config_t* tbc = &kbd_system.core1.tb_config;
    tb_motion_config.version = CONFIG_VERSION;
    tb_motion_config.cpi_multiplier = tbc->cpi / CPI_BASE;
    tb_motion_config.scroll_scale = tbc->scroll_scale;
    tb_motion_config.scroll_quad_weight = tbc->scroll_quad_weight;
    tb_motion_config.delta_scale = tbc->delta_scale;
    tb_motion_config.delta_quad_weight = tbc->delta_quad_weight;
    memcpy(data, &tb_motion_config, sizeof(tb_motion_config_t));
}

void apply_config_screen_data_tb() {
#ifdef KBD_NODE_AP
    uint8_t* data = fd->data;
#else
    uint8_t si = get_screen_index(THIS_SCREEN);
    uint8_t* data = kbd_system.core1.flash_data[si];
#endif
    if(data[0]!=CONFIG_VERSION) return;

    memcpy(&tb_motion_config, data, sizeof(tb_motion_config_t));

    kbd_tb_config_t* tbc = &kbd_system.core1.tb_config;
    tbc->cpi = CPI_BASE * tb_motion_config.cpi_multiplier;
    tbc->scroll_scale = tb_motion_config.scroll_scale;
    tbc->scroll_quad_weight = tb_motion_config.scroll_quad_weight;
    tbc->delta_scale = tb_motion_config.delta_scale;
    tbc->delta_quad_weight = tb_motion_config.delta_quad_weight;

#ifdef KBD_NODE_RIGHT
    // must clearout the motion registers before setting the CPI
    int16_t dx,dy;
    bool on_surface;
    tb_check_motion(kbd_hw.tb, &on_surface, &dx, &dy); // make a dummy call to clear motions
    tb_set_cpi(kbd_hw.tb, tbc->cpi);
#endif
}
