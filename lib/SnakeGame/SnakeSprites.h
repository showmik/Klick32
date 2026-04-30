#pragma once
#include <Arduino.h>

// ─── SnakeSprites.h ───────────────────────────────────────────────────────────
// Bitmap format: MSB-first. 8x8 sprites (1 byte per row).

// Normal Apple - A classic fruit with a stem
static const uint8_t PROGMEM spr_snake_apple[4] = {
    0x60,
    0xF0,
    0xF0,
    0x60,
};

// Bonus Apple - A shiny star or diamond
static const uint8_t PROGMEM spr_snake_bonus[8] = {
    0x3C,
    0x7E,
    0xFF,
    0xFF,
    0xFF,
    0xFF,
    0x7E,
    0x3C,
};

// Poison Apple - A skull and crossbones
static const uint8_t PROGMEM spr_snake_poison[4] = {
    0x90,
    0x60,
    0x60,
    0x90,
};

// Wall Block - 8x8 (Centered, overlaps block edges visually)
static const uint8_t PROGMEM spr_snake_wall[8] = {
    0xFF,
    0x81,
    0xBD,
    0xBD,
    0xBD,
    0xBD,
    0x81,
    0xFF,
};