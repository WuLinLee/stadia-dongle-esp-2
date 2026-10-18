#include "button_actions.h"

#include "bridge.h"
#include "config_store.h"
#include "hid_extra.h"
#include "mouse_mode.h"
#include "web_server.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

static QueueHandle_t s_queue;
static const char *TAG = "ACT";

typedef struct {
    uint8_t gamepad_index;
    stadia_controller_state_t state;
} button_action_msg_t;

typedef struct {
    bool physical_pressed;
    bool output_active;
    uint8_t action;
} action_button_state_t;

static uint8_t action_to_key(uint8_t action)
{
    switch ((dongle_action_t)action) {
    case DONGLE_ACTION_KEY_F12: return 0x45;
    case DONGLE_ACTION_KEY_F13: return 0x68;
    case DONGLE_ACTION_KEY_F14: return 0x69;
    case DONGLE_ACTION_KEY_PRINTSCREEN: return 0x46;
    case DONGLE_ACTION_KEY_ESCAPE: return 0x29;
    case DONGLE_ACTION_KEY_SPACE: return 0x2c;
    case DONGLE_ACTION_KEY_ENTER: return 0x28;
    case DONGLE_ACTION_KEY_F15: return 0x6a;
    case DONGLE_ACTION_KEY_F16: return 0x6b;
    case DONGLE_ACTION_KEY_F17: return 0x6c;
    case DONGLE_ACTION_KEY_F18: return 0x6d;
    case DONGLE_ACTION_KEY_F19: return 0x6e;
    case DONGLE_ACTION_KEY_F20: return 0x6f;
    case DONGLE_ACTION_KEY_F21: return 0x70;
    case DONGLE_ACTION_KEY_F22: return 0x71;
    case DONGLE_ACTION_KEY_F23: return 0x72;
    case DONGLE_ACTION_KEY_F24: return 0x73;
    case DONGLE_ACTION_KEY_TAB: return 0x2b;
    case DONGLE_ACTION_KEY_BACKSPACE: return 0x2a;
    case DONGLE_ACTION_KEY_INSERT: return 0x49;
    case DONGLE_ACTION_KEY_DELETE: return 0x4c;
    case DONGLE_ACTION_KEY_HOME: return 0x4a;
    case DONGLE_ACTION_KEY_END: return 0x4d;
    case DONGLE_ACTION_KEY_PAGE_UP: return 0x4b;
    case DONGLE_ACTION_KEY_PAGE_DOWN: return 0x4e;
    case DONGLE_ACTION_KEY_ARROW_UP: return 0x52;
    case DONGLE_ACTION_KEY_ARROW_DOWN: return 0x51;
    case DONGLE_ACTION_KEY_ARROW_LEFT: return 0x50;
    case DONGLE_ACTION_KEY_ARROW_RIGHT: return 0x4f;
    default: return 0;
    }
}

static uint16_t action_to_consumer(uint8_t action)
{
    switch ((dongle_action_t)action) {
    case DONGLE_ACTION_CONSUMER_VOLUME_UP: return 0x00e9;
    case DONGLE_ACTION_CONSUMER_VOLUME_DOWN: return 0x00ea;
    case DONGLE_ACTION_CONSUMER_PLAY_PAUSE: return 0x00cd;
    case DONGLE_ACTION_CONSUMER_MUTE: return 0x00e2;
    case DONGLE_ACTION_CONSUMER_SCAN_NEXT: return 0x00b5;
    case DONGLE_ACTION_CONSUMER_SCAN_PREV: return 0x00b6;
    default: return 0;
    }
}

static uint32_t action_to_gamepad_button(uint8_t action)
{
    switch ((dongle_action_t)action) {
    case DONGLE_ACTION_GAMEPAD_A: return BRIDGE_GAMEPAD_BUTTON_A;
    case DONGLE_ACTION_GAMEPAD_B: return BRIDGE_GAMEPAD_BUTTON_B;
    case DONGLE_ACTION_GAMEPAD_X: return BRIDGE_GAMEPAD_BUTTON_X;
    case DONGLE_ACTION_GAMEPAD_Y: return BRIDGE_GAMEPAD_BUTTON_Y;
    case DONGLE_ACTION_GAMEPAD_LB: return BRIDGE_GAMEPAD_BUTTON_LB;
    case DONGLE_ACTION_GAMEPAD_RB: return BRIDGE_GAMEPAD_BUTTON_RB;
    case DONGLE_ACTION_GAMEPAD_START: return BRIDGE_GAMEPAD_BUTTON_START;
    case DONGLE_ACTION_GAMEPAD_BACK: return BRIDGE_GAMEPAD_BUTTON_BACK;
    case DONGLE_ACTION_GAMEPAD_L3: return BRIDGE_GAMEPAD_BUTTON_LS;
    case DONGLE_ACTION_GAMEPAD_R3: return BRIDGE_GAMEPAD_BUTTON_RS;
    case DONGLE_ACTION_GAMEPAD_GUIDE: return BRIDGE_GAMEPAD_BUTTON_GUIDE;
    case DONGLE_ACTION_GAMEPAD_DPAD_UP: return BRIDGE_GAMEPAD_BUTTON_DPAD_UP;
    case DONGLE_ACTION_GAMEPAD_DPAD_DOWN: return BRIDGE_GAMEPAD_BUTTON_DPAD_DOWN;
    case DONGLE_ACTION_GAMEPAD_DPAD_LEFT: return BRIDGE_GAMEPAD_BUTTON_DPAD_LEFT;
    case DONGLE_ACTION_GAMEPAD_DPAD_RIGHT: return BRIDGE_GAMEPAD_BUTTON_DPAD_RIGHT;
    case DONGLE_ACTION_GAMEPAD_LT: return BRIDGE_GAMEPAD_BUTTON_LT;
    case DONGLE_ACTION_GAMEPAD_RT: return BRIDGE_GAMEPAD_BUTTON_RT;
    default: return 0;
    }
}

static uint8_t action_slot(uint8_t gamepad_index, bool capture)
{
    return (uint8_t)(gamepad_index * 2U + (capture ? 1U : 0U));
}

static uint8_t s_gamepad_action_refs[DONGLE_MAX_CONTROLLERS][DONGLE_ACTION_GAMEPAD_RT + 1];

static bool set_action_output(uint8_t gamepad_index, uint8_t slot,
                              uint8_t action, bool pressed)
{
    uint8_t key = action_to_key(action);
    uint16_t consumer = action_to_consumer(action);
    uint32_t gamepad_button = action_to_gamepad_button(action);

    if (key) return hid_extra_set_keyboard_slot(slot, 0, key, pressed);
    if (consumer) return hid_extra_set_consumer_slot(slot, consumer, pressed);

    if (gamepad_button) {
        if (pressed && mouse_mode_is_active(gamepad_index)) {
            ESP_LOGI(TAG, "usb %u skipping XInput action %s while in mouse mode",
                     gamepad_index, config_store_action_name(action));
            return false;
        }
        uint8_t *refs = &s_gamepad_action_refs[gamepad_index][action];
        if (pressed) {
            if ((*refs)++ == 0) bridge_set_virtual_gamepad_button(gamepad_index, gamepad_button, true);
        } else if (*refs > 0 && --(*refs) == 0) {
            bridge_set_virtual_gamepad_button(gamepad_index, gamepad_button, false);
        }
        ESP_LOGI(TAG, "usb %u XInput action %s %s", gamepad_index,
                 config_store_action_name(action), pressed ? "down" : "up");
        return true;
    }

    if (pressed && action == DONGLE_ACTION_START_WEBUI) {
        ESP_LOGI(TAG, "usb %u action start_webui", gamepad_index);
        web_server_start(true);
        return true;
    }
    if (pressed && action == DONGLE_ACTION_REMOTE_WAKE_ONLY) {
        bool sent = hid_extra_remote_wakeup();
        ESP_LOGI(TAG, "usb %u action remote_wake_only sent=%s",
                 gamepad_index, sent ? "yes" : "no");
        return true;
    }
    return false;
}

static void start_button(action_button_state_t *btn, uint8_t gamepad_index,
                         uint8_t slot, uint8_t action)
{
    btn->physical_pressed = true;
    btn->action = action;
    btn->output_active = set_action_output(gamepad_index, slot, action, true);
}

static void release_button_output(action_button_state_t *btn, uint8_t gamepad_index,
                                  uint8_t slot)
{
    if (btn->output_active) {
        (void)set_action_output(gamepad_index, slot, btn->action, false);
        btn->output_active = false;
    }
}

static void finish_button(action_button_state_t *btn, uint8_t gamepad_index,
                          uint8_t slot)
{
    release_button_output(btn, gamepad_index, slot);
    btn->physical_pressed = false;
    btn->action = DONGLE_ACTION_NONE;
}

static void update_single_button(action_button_state_t *btn, uint8_t gamepad_index,
                                 uint8_t slot, bool now_pressed, uint8_t mapped_action)
{
    if (now_pressed && !btn->physical_pressed) {
        start_button(btn, gamepad_index, slot, mapped_action);
    } else if (!now_pressed && btn->physical_pressed) {
        finish_button(btn, gamepad_index, slot);
    }
}

static void enter_mouse_combo(action_button_state_t *assistant,
                              action_button_state_t *capture,
                              uint8_t gamepad_index, int64_t now,
                              int64_t *combo_start_us, bool *combo_suppressed,
                              bool *combo_triggered)
{
    release_button_output(assistant, gamepad_index, action_slot(gamepad_index, false));
    release_button_output(capture, gamepad_index, action_slot(gamepad_index, true));
    assistant->physical_pressed = true;
    assistant->action = DONGLE_ACTION_NONE;
    capture->physical_pressed = true;
    capture->action = DONGLE_ACTION_NONE;
    *combo_suppressed = true;
    *combo_start_us = now;
    *combo_triggered = false;
    ESP_LOGI(TAG, "usb %u mouse combo started; extra-button outputs suppressed", gamepad_index);
}

static void update_suppressed_combo(action_button_state_t *btn, bool now_pressed)
{
    btn->physical_pressed = now_pressed;
    btn->action = DONGLE_ACTION_NONE;
    btn->output_active = false;
}

static void button_actions_task(void *arg)
{
    (void)arg;
    if (!s_queue) { vTaskDelete(NULL); return; }
    button_action_msg_t msg = {0};
    action_button_state_t assistant[DONGLE_MAX_CONTROLLERS] = {0};
    action_button_state_t capture[DONGLE_MAX_CONTROLLERS] = {0};
    int64_t setup_combo_start_us[DONGLE_MAX_CONTROLLERS] = {0};
    int64_t mouse_combo_start_us[DONGLE_MAX_CONTROLLERS] = {0};
    bool mouse_combo_suppressed[DONGLE_MAX_CONTROLLERS] = {0};
    bool mouse_combo_triggered[DONGLE_MAX_CONTROLLERS] = {0};

    while (1) {
        dongle_config_t cfg;
        config_store_get(&cfg);

        if (xQueueReceive(s_queue, &msg, pdMS_TO_TICKS(25)) == pdTRUE) {
            uint8_t idx = msg.gamepad_index < DONGLE_MAX_CONTROLLERS ? msg.gamepad_index : 0;
            stadia_controller_state_t st = msg.state;
            int64_t now = esp_timer_get_time();

            bool setup_combo = st.stadia && st.options && st.menu;
            if (setup_combo) {
                if (!setup_combo_start_us[idx]) setup_combo_start_us[idx] = now;
                if (now - setup_combo_start_us[idx] > 5000000) {
                    web_server_start(true);
                    setup_combo_start_us[idx] = now;
                }
            } else {
                setup_combo_start_us[idx] = 0;
            }

            dongle_status_t status;
            dongle_state_get_status(&status);
            if (status.usb_suspended && (st.a || st.assistant || st.capture)) {
                hid_extra_remote_wakeup();
            }

            bool mouse_combo = st.assistant && st.capture;
            if (mouse_combo_suppressed[idx]) {
                update_suppressed_combo(&assistant[idx], st.assistant);
                update_suppressed_combo(&capture[idx], st.capture);
                if (!mouse_combo) {
                    /* The chord must stay continuously held for two seconds. */
                    mouse_combo_start_us[idx] = 0;
                    mouse_combo_triggered[idx] = false;
                }
                if (!st.assistant && !st.capture) {
                    mouse_combo_suppressed[idx] = false;
                    ESP_LOGI(TAG, "usb %u mouse combo released", idx);
                } else if (mouse_combo && mouse_combo_start_us[idx] == 0 &&
                           !mouse_combo_triggered[idx]) {
                    mouse_combo_start_us[idx] = now;
                }
            } else if (mouse_combo) {
                enter_mouse_combo(&assistant[idx], &capture[idx], idx, now,
                                  &mouse_combo_start_us[idx],
                                  &mouse_combo_suppressed[idx],
                                  &mouse_combo_triggered[idx]);
            } else {
                update_single_button(&assistant[idx], idx, action_slot(idx, false),
                                     st.assistant, cfg.assistant_action);
                update_single_button(&capture[idx], idx, action_slot(idx, true),
                                     st.capture, cfg.capture_action);
            }
        }

        for (uint8_t i = 0; i < DONGLE_MAX_CONTROLLERS; i++) {
            bool combo_pending = mouse_combo_start_us[i] != 0 && !mouse_combo_triggered[i];
            if (combo_pending && mouse_combo_suppressed[i] &&
                esp_timer_get_time() - mouse_combo_start_us[i] > 2000000) {
                mouse_combo_triggered[i] = true;
                ESP_LOGI(TAG, "usb %u mouse combo held for 2 seconds; toggling mouse mode", i);
                mouse_mode_toggle(i);
            }
        }
        hid_extra_flush_pending();
    }
}

void button_actions_init(void)
{
    s_queue = xQueueCreate(DONGLE_MAX_CONTROLLERS, sizeof(button_action_msg_t));
    configASSERT(s_queue != NULL);
    xTaskCreate(button_actions_task, "button_actions", 3072, NULL, 2, NULL);
}

void button_actions_on_state(uint8_t gamepad_index, const stadia_controller_state_t *state)
{
    if (!s_queue || !state) return;
    button_action_msg_t msg = {
        .gamepad_index = gamepad_index,
        .state = *state,
    };
    if (xQueueSendToBack(s_queue, &msg, 0) != pdTRUE) {
        button_action_msg_t stale;
        xQueueReceive(s_queue, &stale, 0);
        xQueueSendToBack(s_queue, &msg, 0);
    }
}
