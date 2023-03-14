#include "class/hid/hid.h"

#include "key_layout.h"
#include "input_processor.h"

#define KEY_PRESS_MAX 16

static uint8_t key_layout_read(const uint8_t left[KEY_ROW_COUNT],
                        const uint8_t right[KEY_ROW_COUNT],
                        const uint8_t* key_press[KEY_PRESS_MAX]) {
    int row,col,k=0;
    uint8_t v;

    for(row=0
            ; k<KEY_PRESS_MAX && row<KEY_ROW_COUNT
            ; row++) {
        for(col=KEY_COL_COUNT-1, v=left[row]
                ; v>0 && k<KEY_PRESS_MAX && col>=0
                ; col--, v>>=1) {
            if(v & 1) key_press[k++] = key_layout[row][col];
        }

        for(col=KEY_COL_COUNT-1, v=right[row]
                ; v>0 && k<KEY_PRESS_MAX && col>=0
                ; col--, v>>=1) {
            if(v & 1) key_press[k++] = key_layout[row][KEY_COL_COUNT+col];
        }
    }
    return k;
}

// update the hid_report_out and return the screen event if any

kbd_screen_event_t execute_input_processor(const uint8_t left_key_press[KEY_ROW_COUNT],
                                           const uint8_t right_key_press[KEY_ROW_COUNT],
                                           const kbd_tb_motion_t* tb_motion) {
    const uint8_t* key_press[KEY_PRESS_MAX];
    uint8_t n = key_layout_read(left_key_press, right_key_press, key_press);

    // TODO
    (void)n;

    return kbd_screen_event_NONE;
}
