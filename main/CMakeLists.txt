if(NOT DEFINED STADIA_RUMBLE_PROFILE)
    set(STADIA_RUMBLE_PROFILE 0 CACHE STRING "Rumble profile: 0=reference, 1=gentle, 2=punch")
endif()
if(NOT STADIA_RUMBLE_PROFILE MATCHES "^[012]$")
    message(FATAL_ERROR "STADIA_RUMBLE_PROFILE must be 0, 1, or 2")
endif()

idf_component_register(
    SRCS "xbox_dev.c" "rumble_map.c" "main.c" "bridge.c" "usb_xbox.c" "ble_central.c"
         "dongle_state.c" "status_led.c" "controller_manager.c"
         "config_store.c" "web_server.c" "captive_portal.c"
         "hid_extra.c" "hid_mouse.c" "usb_config_protocol.c"
         "button_actions.c" "mouse_mode.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash bt esp_timer driver esp_driver_rmt esp_wifi esp_netif esp_http_server app_update esp_app_format esp_coex
    EMBED_TXTFILES "index.html" "webhid_transport.js"
)

target_compile_definitions(${COMPONENT_LIB} PRIVATE STADIA_RUMBLE_PROFILE=${STADIA_RUMBLE_PROFILE})
