#ifndef _USB_HID_H
#define _USB_HID_H

#include <stdint.h>

void usb_hid_idle_task(void);

void usb_hid_task(void);

void usb_hid_init(void);

void set_core0_debug(uint8_t index, uint32_t value);

#endif
