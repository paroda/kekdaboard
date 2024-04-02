#include <stdio.h>

#include "pico/multicore.h"

#include "hw_model.h"
#include "data_model.h"

#include "usb_hid.h"

void core1_main();

void core0_main();

void launch_core1() {
    init_hw_core1();

    core1_main();
}

void load_flash() {
    // load FLASH DATASETS
    init_flash_datasets(kbd_system.core0.flash_datasets);

    init_config_screen_data();

    load_flash_datasets(kbd_system.core0.flash_datasets);
}

int main() {
    // stdio_init_all();

    init_data_model();

    init_hw_core0();

    multicore_launch_core1(launch_core1);

    load_flash();

    usb_hid_init();

    core0_main();
}
