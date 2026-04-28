#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "Config.h"
#include "InputManager.h"
#include "Sound.h"
#include "Battery.h"
#include "GameBase.h"

// ─── OS ───────────────────────────────────────────────────────────────────────
// The main firmware class. Owns all hardware drivers and the game registry.
//
// Usage in main.cpp:
//   OS os;
//   DinoGame dino;
//   void setup() { os.begin(); os.registerGame(&dino); }
//   void loop()  { os.run(); }  // never returns
class OS {
public:
    OS();

    // Initialises display, input, sound, and battery. Call once in setup().
    void begin();

    // Add a game to the menu. Call before run().
    void registerGame(GameBase* game);

    // Enter the main firmware loop. Never returns.
    void run();

private:
    // ── Hardware ──────────────────────────────────────────────────────────────
    U8G2_SH1106_128X64_NONAME_F_HW_I2C _disp;
    InputManager _input;
    Sound        _sound;
    Battery      _batt;

    // ── Game registry ─────────────────────────────────────────────────────────
    static constexpr uint8_t MAX_GAMES = 12;
    GameBase* _games[MAX_GAMES] = {};
    uint8_t   _gameCount        = 0;
    uint8_t   _selected         = 0;

    // ── Cached state ──────────────────────────────────────────────────────────
    uint8_t  _battPct   = 0;
    uint32_t _battTimer = 0;

    // ── Menu rendering helpers ────────────────────────────────────────────────
    void _drawMenu();
    void _drawHeader();
    void _drawGameCard(uint8_t idx);
    void _drawFooter();
};