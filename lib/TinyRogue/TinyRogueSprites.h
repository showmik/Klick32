#pragma once
#include <Arduino.h>

// ─── TinyRogueSprites.h ───────────────────────────────────────────────────────
// Bitmap format: MSB-first. 8x8 sprites (1 byte per row).
// ─── UI Icons (8x8) ───────────────────────────────────────────────────────────

// Heart: A classic rounded heart for HP
static const uint8_t PROGMEM spr_icon_heart[8] = {
    0x66, 0xFF, 0xFF, 0x7E, 0x3C, 0x18, 0x00, 0x00
};

// Sword: An upright broadsword for Attack
static const uint8_t PROGMEM spr_icon_sword[8] = {
    0x03,
    0x07,
    0x0E,
    0x5C,
    0x78,
    0x30,
    0x58,
    0x80,
};

// Shield: A solid heater shield for Defense
static const uint8_t PROGMEM spr_icon_shield[8] = {
    0x7E,
    0x42,
    0x00,
    0x42,
    0x42,
    0x24,
    0x18,
    0x00,
};

// Coin: A gleaming coin for Gold
static const uint8_t PROGMEM spr_icon_coin[8] = {
    0x3C,
    0x46,
    0x9D,
    0xBD,
    0xBD,
    0xB9,
    0x42,
    0x3C,
};

// Floor: Extreme whitespace, just a single pixel anchor
static const uint8_t PROGMEM spr_rogue_floor[8] = {
    0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00
};

// Wall: Crisp, hollow square to define boundaries without visual clutter
static const uint8_t PROGMEM spr_rogue_wall[8] = {
   0x00,
0x7E,
0x42,
0x42,
0x42,
0x42,
0x7E,
0x00,
};

// Player: Sharp, solid diamond
static const uint8_t PROGMEM spr_rogue_player[8] = {
    0x38,
    0x54,
    0x38,
    0x7C,
    0xBA,
    0xBA,
    0x28,
    0x28,
};

// Stairs: Clean descending chevron
static const uint8_t PROGMEM spr_rogue_stairs[8] = {
    0x00,
    0x00,
    0x00,
    0x80,
    0xA0,
    0xA8,
    0xAA,
    0xAA,
};

// Depth: A small skull to indicate dungeon depth and danger
static const uint8_t PROGMEM spr_icon_depth[8] = {
    0x3C,
    0x7E,
    0xFF,
    0x99,
    0x99,
    0x7E,
    0x5A,
    0x00,
};

// Chest: Minimalist box with a center latch
static const uint8_t PROGMEM spr_rogue_chest[8] = {
    0x7E,
    0xDD,
    0xDD,
    0xF7,
    0x89,
    0x81,
    0x81,
    0xFF,
};

// Rat: Low-profile, grounded triangle
static const uint8_t PROGMEM spr_rogue_rat[8] = {
    0x00,
    0x1B,
    0x0E,
    0x34,
    0x7E,
    0x2A,
    0x00,
    0x00,
};

// Goblin: Jagged, distinct silhouette
static const uint8_t PROGMEM spr_rogue_goblin[8] = {
    0x1C,
    0x2A,
    0x1C,
    0x22,
    0x5D,
    0x1C,
    0x14,
    0x14,
};

// Bat: Sharp V-shaped wings, hangs near the top of the tile
static const uint8_t PROGMEM spr_rogue_bat[8] = {
    0x24,
    0x99,
    0xE7,
    0x7E,
    0x18,
    0x24,
    0x00,
    0x00,
};

// Skeleton: Skull and ribs
static const uint8_t PROGMEM spr_rogue_skeleton[8] = {
    0x1C,
    0x2A,
    0x3E,
    0x14,
    0x3E,
    0x49,
    0x1C,
    0x22,
};

// Merchant: A hooded figure with a gleaming coin
static const uint8_t PROGMEM spr_rogue_merchant[8] = {
    0x7E,
    0xF2,
    0xB2,
    0x3E,
    0x41,
    0x5D,
    0x1C,
    0x22,
};