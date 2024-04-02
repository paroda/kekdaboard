#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/multicore.h"

#include "hw_model.h"
#include "data_model.h"

void core1_main();

void core0_main();

void launch_core1() {
    init_hw_core1();

    core1_main();
}

int main() {
    // stdio_init_all();

    init_data_model();

    init_hw_core0();

    multicore_launch_core1(launch_core1);

    core0_main();
}
