#pragma once
#include <stdint.h>

namespace ChessSprites {

    // Refined 8x8 sprites. 1 byte per row.
    // Designed with a 5x6 inner bounding box (x=1..5, y=1..6)
    // to allow a 1-pixel outline without bleeding across adjacent tiles.
    
    const uint8_t pawn[] = {
        0b00000000,
        0b00000000,
        0b00010000,
        0b00111000,
        0b00010000,
        0b00111000,
        0b01111100,
        0b00000000
    };

    const uint8_t knight[] = {
        0b00000000,
        0b00011000,
        0b01111000,
        0b01101000,
        0b00011000,
        0b00111100,
        0b01111100,
        0b00000000
    };

    const uint8_t bishop[] = {
        0b00000000,
        0b00010000,
        0b00111000,
        0b00101000,
        0b00111000,
        0b00111000,
        0b01111100,
        0b00000000
    };

    const uint8_t rook[] = {
        0b00000000,
        0b01010100,
        0b01111100,
        0b00111000,
        0b00111000,
        0b01111100,
        0b01111100,
        0b00000000
    };

    const uint8_t queen[] = {
        0b00000000,
        0b01010100,
        0b01101100,
        0b01111100,
        0b00111000,
        0b00111000,
        0b01111100,
        0b00000000
    };

    const uint8_t king[] = {
        0b00000000,
        0b00010000,
        0b00111000,
        0b00010000,
        0b01111100,
        0b01111100,
        0b01111100,
        0b00000000
    };

}

