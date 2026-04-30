/**
 * Game Console Firmware — Main Entry Point
 *
 * Registering a new game:
 *   1. Create lib/YourGame/YourGame.h + YourGame.cpp
 *   2. #include "YourGame.h" below
 *   3. Declare a global instance: YourGame yourGame;
 *   4. Call os.registerGame(&yourGame);
 *
 * Removing a game: delete its lib/ folder and the three lines above.
 * PlatformIO's LDF discovers lib/ automatically — no platformio.ini changes needed.
 *
 * Build: PlatformIO, Arduino framework, ESP32-S3
 */

#include <Arduino.h>
#include "OS.h"

// ─── Game includes ────────────────────────────────────────────────────────────
// Each game lives in lib/<GameName>/ as a self-contained PlatformIO library.
#include "DinoGame.h"
#include "SnakeGame.h"
// #include "PongGame.h"
// ... add more here

// ─── Global instances ────────────────────────────────────────────────────────
OS       os;
DinoGame dino;
SnakeGame snake;
// PongGame  pong;

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    os.begin();

    os.registerGame(&dino);
    os.registerGame(&snake);
    // os.registerGame(&pong);

    os.run();   // never returns
}

void loop() {}  // OS::run() never returns; loop() never called