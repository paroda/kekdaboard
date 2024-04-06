#include <stdio.h>

#include "pico/multicore.h"

#include "hw_model.h"
#include "data_model.h"

#include "usb_hid.h"

void core1_main();

void core0_main();

void proc_core1() {

    init_hw_core1();

    // load FLASH DATASETS
    init_flash_datasets(kbd_system.core1.flash_datasets);
    init_config_screen_data();
    load_flash_datasets(kbd_system.core1.flash_datasets);

    usb_hid_init();

    core1_main();
}

void proc_core0() {

    init_hw_core0();

    core0_main();
}

int main() {
    // stdio_init_all();
    // sleep_ms(1000);
    // printf("booting up access point node\n");

    init_data_model();

    multicore_launch_core1(proc_core1);

    proc_core0();
}
