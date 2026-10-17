#pragma once
#include <stdint.h>
#include <stddef.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "dongle_config.h"
#include "dongle_state.h"

// Set to 1 to enable verbose HID report logging on BLE and USB paths.
#define DONGLE_DEBUG 0

// ble_to_usb: 20-byte Xbox 360 input reports translated from Stadia BLE notifications
// usb_to_ble: 4-byte Stadia rumble payloads from Xbox 360 rumble output reports
extern QueueHandle_t ble_to_usb_queue;
extern QueueHandle_t usb_to_ble_queue;

typedef struct {
    uint8_t gamepad_index;
    uint8_t data[DONGLE_XBOX_REPORT_SIZE];
    size_t len;
} ble_to_usb_msg_t;

typedef struct {
    uint8_t gamepad_index;
    uint8_t data[DONGLE_RUMBLE_REPORT_SIZE];
    size_t len;
} usb_to_ble_msg_t;

typedef enum {
    BRIDGE_GAMEPAD_BUTTON_DPAD_UP    = 1u << 0,
    BRIDGE_GAMEPAD_BUTTON_DPAD_DOWN  = 1u << 1,
    BRIDGE_GAMEPAD_BUTTON_DPAD_LEFT  = 1u << 2,
    BRIDGE_GAMEPAD_BUTTON_DPAD_RIGHT = 1u << 3,
    BRIDGE_GAMEPAD_BUTTON_START      = 1u << 4,
    BRIDGE_GAMEPAD_BUTTON_BACK       = 1u << 5,
    BRIDGE_GAMEPAD_BUTTON_LS         = 1u << 6,
    BRIDGE_GAMEPAD_BUTTON_RS         = 1u << 7,
    BRIDGE_GAMEPAD_BUTTON_LB         = 1u << 8,
    BRIDGE_GAMEPAD_BUTTON_RB         = 1u << 9,
    BRIDGE_GAMEPAD_BUTTON_GUIDE      = 1u << 10,
    BRIDGE_GAMEPAD_BUTTON_A          = 1u << 11,
    BRIDGE_GAMEPAD_BUTTON_B          = 1u << 12,
    BRIDGE_GAMEPAD_BUTTON_X          = 1u << 13,
    BRIDGE_GAMEPAD_BUTTON_Y          = 1u << 14,
    BRIDGE_GAMEPAD_BUTTON_LT         = 1u << 15,
    BRIDGE_GAMEPAD_BUTTON_RT         = 1u << 16,
} bridge_gamepad_button_t;

void bridge_init(void);
void bridge_send_neutral(uint8_t gamepad_index);
void bridge_send_xbox_report(uint8_t gamepad_index, const uint8_t *report, size_t len);
void bridge_set_virtual_gamepad_button(uint8_t gamepad_index, uint32_t button, bool pressed);
void stadia_parse_state(const uint8_t *stadia, size_t len, stadia_controller_state_t *out);
void stadia_to_xbox360(const uint8_t *stadia, uint8_t *xbox);
void stadia_pressed_names(const stadia_controller_state_t *state, char *out, size_t out_len);
