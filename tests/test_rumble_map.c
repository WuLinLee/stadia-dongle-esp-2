#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "rumble_map.h"

static uint16_t from_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

int main(void)
{
    uint8_t packet[4] = {0};
    assert(rumble_map_one(0) == 0);
    assert(rumble_map_one(255) == 65535);
    for (unsigned i = 1; i < 256; ++i) {
        uint16_t previous = rumble_map_one((uint8_t)(i - 1));
        uint16_t current = rumble_map_one((uint8_t)i);
        assert(current >= previous);
#if STADIA_RUMBLE_PROFILE == 0
        assert(current == (uint16_t)(i * 257u));
#else
        assert(current > (uint16_t)(i * 257u));
#endif
    }
    rumble_map_xbox360(128, 64, packet);
    assert(from_le(&packet[0]) == rumble_map_one(128));
    assert(from_le(&packet[2]) == rumble_map_one(64));
    rumble_map_xbox360(0, 0, packet);
    assert(from_le(&packet[0]) == 0 && from_le(&packet[2]) == 0);
    puts("rumble-map tests passed");
    return 0;
}
