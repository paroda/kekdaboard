#include "pico/multicore.h"

#include "data_model.h"
#include "hw_model.h"

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

  init_data_model();

  multicore_launch_core1(proc_core1);

  proc_core0();
}
