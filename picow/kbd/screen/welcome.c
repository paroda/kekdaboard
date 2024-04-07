#include <stdio.h>
#include <string.h>

#include "../hw_model.h"
#include "../data_model.h"

#define THIS_SCREEN kbd_info_screen_welcome

#ifdef KBD_NODE_AP

static uint8_t next_si = 0;

void handle_screen_event_welcome(kbd_event_t event) {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* lreq = c->left_task_request;
    uint8_t* rreq = c->right_task_request;
    uint8_t* lres = c->left_task_response;
    uint8_t* rres = c->right_task_response;

    if(is_nav_event(event)) return;

    uint8_t si;
    kbd_screen_t screen;
    flash_dataset_t* fd;
    uint8_t* left_fd_pos = c->left_flash_data_pos;
    uint8_t* right_fd_pos = c->right_flash_data_pos;

    switch(event) {
    case kbd_event_NONE: // when idle, sync config to left/right, one screen at a time
        si = next_si;
        screen = kbd_config_screens[si];
        fd = c->flash_datasets[si];
        if(fd->pos != left_fd_pos[si]) {
            init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
            lreq[2] = screen;
            lreq[3] = fd->pos;
            memcpy(lreq+4, fd->data, FLASH_DATASET_SIZE);
        }
        if(fd->pos != right_fd_pos[si]) {
            init_task_request(rreq, &c->right_task_request_ts, THIS_SCREEN);
            rreq[2] = screen;
            rreq[3] = fd->pos;
            memcpy(rreq+4, fd->data, FLASH_DATASET_SIZE);
        }
        next_si = si+1<KBD_CONFIG_SCREEN_COUNT ? si+1 : 0;
        break;
    case kbd_screen_event_INIT:
        init_task_request(lreq, &c->left_task_request_ts, THIS_SCREEN);
        lreq[2] = 1;
        break;
    case kbd_screen_event_RESPONSE:
        if(lres[0] && lres[1]==THIS_SCREEN && lres[2]==1) {
            si = get_screen_index(lres[4]);
            left_fd_pos[si] = lres[5];
        }
        if(rres[0] && rres[1]==THIS_SCREEN && rres[2]==1) {
            si = get_screen_index(rres[4]);
            right_fd_pos[si] = rres[5];
        }
        break;
    default: break;
    }
}

#else // LEFT/RIGHT

void work_screen_task_welcome() {
    kbd_system_core1_t* c = &kbd_system.core1;
    uint8_t* req = c->task_request;
    uint8_t* res = c->task_response;

    // req[2] is either a config screen (0x1#) or a command (0x0#)

    kbd_screen_t screen = req[2];
    bool config = is_config_screen(screen);
    uint8_t si = get_screen_index(screen);
    uint8_t pos = req[3];
    if(config && si<KBD_CONFIG_SCREEN_COUNT) {
        c->flash_data_pos[si] = pos;
        memcpy(c->flash_data[si], req+4, KBD_TASK_DATA_SIZE);
        res[2] = 1;
        res[3] = 2;
        res[4] = screen;
        res[5] = pos;
    } else {
#ifdef KBD_NODE_LEFT
        switch(req[2]) {
        case 1: // init
            lcd_show_welcome();
            break;
        default: break;
        }
#endif
    }
}

#endif
