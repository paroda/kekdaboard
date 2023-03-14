#ifndef _INPUT_PROCESSOR_H
#define _INPUT_PROCESSOR_H

#include "hw_config.h"
#include "data_model.h"

kbd_screen_event_t execute_input_processor(const uint8_t left_key_press[KEY_ROW_COUNT],
                                           const uint8_t right_key_press[KEY_ROW_COUNT],
                                           const kbd_tb_motion_t* tb_motion);

#endif
