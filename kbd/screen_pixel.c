#include <stdio.h>
#include <string.h>

#include "hw_model.h"
#include "screen_processor.h"

#define CONFIG_VERSION 0x01

typedef struct {
    uint8_t version;
    uint8_t color_red;
    uint8_t color_green;
    uint8_t color_blue;
    uint8_t anim_style;
    uint8_t anim_cycles;
} pixel_config_t;

static pixel_config_t pixel_config;
static flash_dataset_t* fd;

static bool dirty;

void execute_screen_pixel(kbd_event_t event) {

}

void respond_screen_pixel() {

}

void init_config_screen_default_pixel() {

}

void apply_config_screen_pixel() {
    
}