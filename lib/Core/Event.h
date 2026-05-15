#pragma once
#include <stdint.h>

namespace Event {
    typedef uint8_t ID;

    // Standard engine/UI events
    static constexpr ID NONE       = 0;
    static constexpr ID PAUSE      = 1;
    static constexpr ID RESUME     = 2;
    static constexpr ID QUIT       = 3;

    // Game-specific events start here
    static constexpr ID GAME_OVER  = 10;
    static constexpr ID RESTART    = 11;
    static constexpr ID CUSTOM_1   = 12;
    static constexpr ID CUSTOM_2   = 13;
    static constexpr ID CUSTOM_3   = 14;
    static constexpr ID CUSTOM_4   = 15;
}
