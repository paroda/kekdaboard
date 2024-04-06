#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/multicore.h"

#include "hw_model.h"
#include "data_model.h"

void core1_main();

void core0_main();

void proc_core1() {

    init_hw_core1();

    core1_main();
}

void proc_core0() {

    init_hw_core0();

    core0_main();
}

int main() {
    stdio_init_all();
    sleep_ms(1000);
    printf("booting up right node\n");

    init_data_model();

    multicore_launch_core1(proc_core1);

    proc_core0();
}
