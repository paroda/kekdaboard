#ifndef _KBD_PROCESS_H
#define _KBD_PROCESS_H

#include "data_model.h"

/*
 * Process Overview
 *
 * - set my_side (main_xxx.c)
 * - setup data_model
 * - start core1
 *   - setup ball scroll listener (only right side)
 *   - setup uart listener
 *   - loop tasks
 *     - scan keys 7x7
 *     - scan ball btn (only right side)
 *     - init transfer cycle
 * - initiate usb - setup role USB on connect
 * - loop tasks
 *   - not ready ->
 *     - not peer-ready ->
 *       - ping AUX if role USB
 *     - peer-ready ->
 *       - setup role AUX if NONE
 *       - set ready
 *   - ready ->
 *     - role USB ->
 *       - process inputs and update system_state
 *     - side left ->
 *       - write LCD based on system_state
 *     - side right ->
 *       - write LED based on system_state
 */

void kbd_process(void);


#endif
