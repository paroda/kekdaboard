#include "main.h"

#include "pico/multicore.h"

#include "data_model.h"
#include "hw_model.h"

#ifdef KBD_NODE_AP
#include "usb_hid.h"
#endif

void core1_main();

void core0_main();

void proc_core1() {

  init_hw_core1();

#ifdef KBD_NODE_AP
  // load FLASH DATASETS
  init_flash_datasets(kbd_system.core1.flash_datasets);
  init_config_screen_data();
  load_flash_datasets(kbd_system.core1.flash_datasets);

  usb_hid_init();
#endif

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
