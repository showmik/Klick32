/**
 * Game Console Firmware — Main Entry Point
 *
 * This file bootstraps the OS and registers available games.
 * The OS handles display, input, sound, battery, and the game menu.
 * Each game is a separate class inheriting from GameBase.
 *
 * Build: PlatformIO, Arduino framework
 */

#include <Arduino.h>
#include "OS.h"
#include "DinoGame.h"

// ─── Global Instances ───────────────────────────────────────────────────────
OS       os;        // The firmware OS
DinoGame dino;       // Chrome-style endless runner

// ─── Setup ───────────────────────────────────────────────────────────────────
void setup() {
    // Initialize the OS (display, input, sound, battery)
    os.begin();

    // Register all available games
    os.registerGame(&dino);

    // Enter the main firmware loop (never returns)
    os.run();
}

// ─── Loop ───────────────────────────────────────────────────────────────────
// The OS::run() method never returns, so loop() is never called.
// Include an empty loop to satisfy the Arduino framework.
void loop() {
    // Empty — firmware runs entirely within OS::run()
}
