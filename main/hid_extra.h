#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "device/usbd_pvt.h"

#define HID_EXTRA_REPORT_DESC_LEN 113

void hid_extra_init(void);
#define HID_EXTRA_HOLD_SLOTS 4

bool hid_extra_set_keyboard_slot(uint8_t slot, uint8_t modifier, uint8_t keycode, bool pressed);
bool hid_extra_set_consumer_slot(uint8_t slot, uint16_t usage, bool pressed);
void hid_extra_flush_pending(void);
bool hid_extra_remote_wakeup(void);
const uint8_t *hid_extra_report_descriptor(void);
usbd_class_driver_t const *hid_extra_class_driver(void);
