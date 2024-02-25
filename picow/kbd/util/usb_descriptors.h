/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef USB_DESCRIPTORS_H_
#define USB_DESCRIPTORS_H_

#include "tusb.h"

enum
{
    REPORT_ID_KEYBOARD = 1,
    REPORT_ID_MOUSE,
    REPORT_ID_CONSUMER_CONTROL,
    REPORT_ID_GAMEPAD,
    REPORT_ID_COUNT
};

enum
{
    ITF_NUM_HID1,
    ITF_NUM_HID2,
    ITF_NUM_TOTAL
};

/*
 * Modified version of TUD_HID_REPORT_DESC_MOUSE
 * It uses 16 bits for delta-X, delta-Y, scroll and pan
 */

// Mouse Report Descriptor Template
#define KBD_HID_REPORT_DESC_MOUSE(...) \
  HID_USAGE_PAGE ( HID_USAGE_PAGE_DESKTOP      )                   ,\
  HID_USAGE      ( HID_USAGE_DESKTOP_MOUSE     )                   ,\
  HID_COLLECTION ( HID_COLLECTION_APPLICATION  )                   ,\
    /* Report ID if any */\
    __VA_ARGS__ \
    HID_USAGE      ( HID_USAGE_DESKTOP_POINTER )                   ,\
    HID_COLLECTION ( HID_COLLECTION_PHYSICAL   )                   ,\
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_BUTTON  )                   ,\
        HID_USAGE_MIN   ( 1                                      ) ,\
        HID_USAGE_MAX   ( 5                                      ) ,\
        HID_LOGICAL_MIN ( 0                                      ) ,\
        HID_LOGICAL_MAX ( 1                                      ) ,\
        /* Left, Right, Middle, Backward, Forward buttons */ \
        HID_REPORT_COUNT( 5                                      ) ,\
        HID_REPORT_SIZE ( 1                                      ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_ABSOLUTE ) ,\
        /* 3 bit padding */ \
        HID_REPORT_COUNT( 1                                      ) ,\
        HID_REPORT_SIZE ( 3                                      ) ,\
        HID_INPUT       ( HID_CONSTANT                           ) ,\
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_DESKTOP )                   ,\
        /* X, Y position [-32767, 32767] */ \
        HID_USAGE       ( HID_USAGE_DESKTOP_X                    ) ,\
        HID_USAGE       ( HID_USAGE_DESKTOP_Y                    ) ,\
        HID_LOGICAL_MIN_N ( 0x8001, 2                            ) ,\
        HID_LOGICAL_MAX_N ( 0x7fff, 2                            ) ,\
        HID_REPORT_COUNT( 2                                      ) ,\
        HID_REPORT_SIZE ( 16                                     ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ) ,\
        /* Verital wheel scroll [-32767, 32767] */ \
        HID_USAGE       ( HID_USAGE_DESKTOP_WHEEL                ) ,\
        HID_LOGICAL_MIN_N ( 0x8001, 2                            ) ,\
        HID_LOGICAL_MAX_N ( 0x7fff, 2                            ) ,\
        HID_REPORT_COUNT( 1                                      ) ,\
        HID_REPORT_SIZE ( 16                                     ) ,\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ) ,\
      HID_USAGE_PAGE  ( HID_USAGE_PAGE_CONSUMER ), \
       /* Horizontal wheel scroll [-32767, 32767] */ \
        HID_USAGE_N     ( HID_USAGE_CONSUMER_AC_PAN, 2           ),\
        HID_LOGICAL_MIN_N ( 0x8001, 2                            ),\
        HID_LOGICAL_MAX_N ( 0x7fff, 2                            ),\
        HID_REPORT_COUNT( 1                                      ),\
        HID_REPORT_SIZE ( 16                                     ),\
        HID_INPUT       ( HID_DATA | HID_VARIABLE | HID_RELATIVE ),\
    HID_COLLECTION_END                                            ,\
  HID_COLLECTION_END \

#endif /* USB_DESCRIPTORS_H_ */
