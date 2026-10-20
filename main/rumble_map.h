#pragma once

#include <stdint.h>

/* Convert Xbox 360 strong/weak motor bytes into 16-bit little-endian Stadia BLE magnitudes. */
#ifndef STADIA_RUMBLE_PROFILE
#define STADIA_RUMBLE_PROFILE 0
#endif

void rumble_map_xbox360(uint8_t strong_8, uint8_t weak_8, uint8_t stadia_le[4]);
uint16_t rumble_map_one(uint8_t input_8);
