#include "screen_processor.h"

#define CONFIG_VERSION 0x01

typedef struct {
    uint8_t backlight;
    uint8_t idle_minutes; // minutes of idleness after which to turn off the backlight
} power_config_t;

void execute_screen_power(kbd_event_t event) {
    (void)event;
    // TODO
}

void respond_screen_power(void) {
    // TODO
}

void init_config_screen_default_power() {
    // No action
}

void apply_config_screen_power() {
    // TODO
}
