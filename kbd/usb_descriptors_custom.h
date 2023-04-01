#ifndef USB_DESCRIPTORS_CUSTOM_H_
#define USB_DESCRIPTORS_CUSTOM_H_

#include "tusb.h"

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


#endif
