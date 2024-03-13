#include <string.h>

#include "../hw_model.h"
#include "../data_model.h"

#ifdef KBD_NODE_AP

void handle_screen_event_scan(kbd_event_t event) {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->left_task_request;

    if(is_nav_event(event)) return;

    init_task_request(req, &c->left_task_request_ts, kbd_info_screen_scan);

    req[2] = (event == kbd_screen_event_INIT) ? 1 : 2;
    req[3] = KEY_ROW_COUNT*2 + sizeof(kbd_tb_motion_t);

    // add scan data
    uint8_t pos=4;
    memcpy(req+pos, c->left_key_press, KEY_ROW_COUNT);
    pos += KEY_ROW_COUNT;
    memcpy(req+pos, c->right_key_press, KEY_ROW_COUNT);
    pos += KEY_ROW_COUNT;
    memcpy(req+pos, &c->tb_motion, sizeof(kbd_tb_motion_t));
}

#endif

#ifdef KBD_NODE_LEFT

uint8_t key_press[KEY_ROW_COUNT][KEY_COL_COUNT*2];

static void key_layout_read(bool init, uint8_t* left_key_press, uint8_t* right_key_press) {
    int row,col;
    uint8_t v, ov;

    for(row=0 ; row<KEY_ROW_COUNT ; row++) {
        for(col=KEY_COL_COUNT-1, v=left_key_press[row] ; col>=0 ; col--, v>>=1) {
            if(init) {
                key_press[row][col] = (v&1) ? 1 : 0;
            } else {
                ov = key_press[row][col] & 1;
                if(((v&1) && ov) || !((v&1) || ov))
                    key_press[row][col] = ov| 2;
                else
                    key_press[row][col] = ov ? 0 : 1;
            }
        }
        for(col=KEY_COL_COUNT-1, v=right_key_press[row] ; col>=0 ; col--, v>>=1) {
            uint8_t col2 = KEY_COL_COUNT+col;
            if(init) {
                key_press[row][col2] = (v&1) ? 1 : 0;
            } else {
                ov = key_press[row][col2] & 1;
                if(((v&1) && ov) || !((v&1) || ov))
                    key_press[row][col2] = ov| 2;
                else
                    key_press[row][col2] = ov ? 0 : 1;
            }
        }
    }
}

static int16_t scale_tb_motion(int32_t x, uint16_t max) {
    x = x / (kbd_system.core0.tb_config.cpi/max);
    return (int16_t) (x > max ? max : x < -max ? -max : x);
}

static void init_screen() {
    lcd_canvas_t* cv = kbd_hw.lcd_body;
    lcd_canvas_clear(cv);

    uint8_t* req = kbd_system.core0.task_request;
    key_layout_read(true, req+4, req+4+KEY_ROW_COUNT);

    // x: 5 (10+5)x7=105 5 10 5 (5+10)x7=105 5
    // y: 15 (10+15)x3=75 5 10 5 (15+10)x3=75 15
    // d: 95, 5-100 100-195, 25-120 120-215

    uint8_t i,j,k,r,c;
    uint16_t x, y, xs[4]={5, 135, 5, 135}, ys[4]={15, 15, 125, 125}, color;
    for(k=0; k<4; k++) {
        r = k<2 ? 0 : 3;
        c = k==0||k==2 ? 0 : 7;
        x = xs[k];
        y = ys[k];
        for(i=0; i<7; i++)
            for(j=0; j<3; j++) {
                color = key_press[r+j][c+i]==1 ? MAGENTA : DARK_GRAY;
                lcd_canvas_rect(cv, x+15*i, y+25*j, 10, 10, color, 1, true);
            }
    }

    lcd_canvas_rect(cv, 115, 5, 10, 190, DARK_GRAY, 1, true);
    lcd_canvas_rect(cv, 25, 95, 190, 10, DARK_GRAY, 1, true);

    kbd_tb_motion_t tbm;
    memcpy(&tbm, req+4+KEY_ROW_COUNT+KEY_ROW_COUNT, sizeof(kbd_tb_motion_t));
    if(!tbm.on_surface) lcd_canvas_rect(cv, 115, 95, 10, 10, RED, 1, true);

    if(tbm.has_motion) {
        int16_t dx = scale_tb_motion(tbm.dx, 85);
        if(dx>0)
            lcd_canvas_rect(cv, 130, 95, dx, 10, YELLOW, 1, true);
        else
            lcd_canvas_rect(cv, 110+dx, 95, -dx, 10, YELLOW, 1, true);

        int16_t dy = scale_tb_motion(tbm.dy, 85);
        if(dy>0)
            lcd_canvas_rect(cv, 115, 110, 10, dy, YELLOW, 1, true);
        else
            lcd_canvas_rect(cv, 115, 90+dy, 10, -dy, YELLOW, 1, true);
    }

    lcd_display_body();
}

static void update_screen() {
    lcd_canvas_t* canvas = lcd_new_canvas(190, 10, DARK_GRAY);
    lcd_canvas_t* cv0 = lcd_new_shared_canvas(canvas->buf, 10, 10, DARK_GRAY);
    lcd_canvas_t* cv1 = lcd_new_shared_canvas(canvas->buf+100, 10, 10, MAGENTA);
    lcd_canvas_t* cv;

    uint8_t* req = kbd_system.core0.task_request;
    key_layout_read(false, req+3, req+3+KEY_ROW_COUNT);

    uint8_t i,j,k,r,c,v;
    uint16_t x, y, xs[4]={5, 135, 5, 135}, ys[4]={55, 55, 165, 165};
    for(k=0; k<4; k++) {
        r = k<2 ? 0 : 3;
        c = k==0||k==2 ? 0 : 7;
        x = xs[k];
        y = ys[k];
        for(i=0; i<7; i++)
            for(j=0; j<3; j++) {
                v = key_press[r+j][c+i];
                cv = v&1 ? cv1 : cv0;
                if(!(v&2))
                    lcd_display_canvas(kbd_hw.lcd, x+15*i, y+25*j, cv);
            }
    }

    kbd_tb_motion_t tbm;
    memcpy(&tbm, req+3+KEY_ROW_COUNT+KEY_ROW_COUNT, sizeof(kbd_tb_motion_t));

    cv = canvas;
    lcd_canvas_clear(cv);


    if(tbm.has_motion) {
        int16_t dx = scale_tb_motion(tbm.dx, 85);
        if(dx>0)
            lcd_canvas_rect(cv, 105, 0, dx, 10, YELLOW, 1, true);
        else
            lcd_canvas_rect(cv, 85+dx, 0, -dx, 10, YELLOW, 1, true);
    }
    lcd_display_canvas(kbd_hw.lcd, 25, 135, cv);

    cv->width = 10; // reuse the canvas as same size, just re-orient
    cv->height = 190;
    lcd_canvas_clear(cv);
    if(tbm.has_motion) {
        int16_t dy = scale_tb_motion(tbm.dy, 85);
        if(dy>0)
            lcd_canvas_rect(cv, 0, 105, 10, dy, YELLOW, 1, true);
        else
            lcd_canvas_rect(cv, 0, 85+dy, 10, -dy, YELLOW, 1, true);
    }
    if(!tbm.on_surface) lcd_canvas_rect(cv, 0, 90, 10, 10, RED, 1, true);
    lcd_display_canvas(kbd_hw.lcd, 115, 45, cv);

    lcd_free_canvas(canvas);
    lcd_free_canvas(cv0);
    lcd_free_canvas(cv1);
}

void work_screen_task_scan() {
    kbd_system_core0_t* c = &kbd_system.core0;
    uint8_t* req = c->task_request;

    switch(req[2]) {
    case 1:
        init_screen();
        break;
    case 2:
        update_screen();
        break;
    default: break;
    }
}

#else

void work_screen_task_scan() {} // no action

#endif
