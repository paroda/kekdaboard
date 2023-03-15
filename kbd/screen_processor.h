#ifndef _SCREEN_PROCESSOR_H
#define _SCREEN_PROCESSOR_H

#include "data_model.h"

typedef bool render_screen_t (kbd_screen_event_t event);

render_screen_t execute_screen_processor;

// render function should return true if it has finished the job
// else returns false to indicate it is busy.
// Only when the renderer is done, the main processor would do any navigation.

render_screen_t render_screen_home;

render_screen_t render_screen_date;

#endif
