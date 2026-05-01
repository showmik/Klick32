#pragma once
#include <Arduino.h>

// ─── TinyRogueSprites.h ───────────────────────────────────────────────────────
// Bitmap format: MSB-first. 8x8 sprites (1 byte per row).

// Floor: Extreme whitespace, just a single pixel anchor
static const uint8_t PROGMEM spr_rogue_floor[8] = {
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00
};

// Wall: Crisp, hollow square to define boundaries without visual clutter
static const uint8_t PROGMEM spr_rogue_wall[8] = {
    0xFF, 0x81, 0x81, 0x81, 0x81, 0x81, 0x81, 0xFF
};

// Player: Sharp, solid diamond
static const uint8_t PROGMEM spr_rogue_player[8] = {
    0x18, 0x3C, 0x7E, 0xFF, 0xFF, 0x7E, 0x3C, 0x18
};

// Stairs: Clean descending chevron
static const uint8_t PROGMEM spr_rogue_stairs[8] = {
    0x00, 0x40, 0x60, 0x70, 0x78, 0x7C, 0x00, 0x00
};

// Chest: Minimalist box with a center latch
static const uint8_t PROGMEM spr_rogue_chest[8] = {
    0x00, 0x7E, 0x42, 0x5A, 0x42, 0x7E, 0x00, 0x00
};

// Rat: Low-profile, grounded triangle
static const uint8_t PROGMEM spr_rogue_rat[8] = {
    0x00, 0x00, 0x00, 0x00, 0x18, 0x3C, 0x7E, 0x00
};

// Goblin: Jagged, distinct silhouette
static const uint8_t PROGMEM spr_rogue_goblin[8] = {
    0x00, 0x24, 0x7E, 0x5A, 0x7E, 0x24, 0x24, 0x00
};