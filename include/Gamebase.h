#pragma once
#include "Console.h"

// ─── GameBase ─────────────────────────────────────────────────────────────────
// Every game inherits from this class and implements all pure-virtual methods.
//
// Lifecycle managed by the OS:
//   1. OS calls onEnter(ctx) when the game is launched.
//   2. OS calls update(ctx) + draw(ctx) every frame while the game is active.
//   3. When isRunning() returns false, the OS calls onExit(ctx) and returns
//      to the menu.
//
// Frame contract:
//   update(ctx) — advance logic, read input, trigger sounds.
//   draw(ctx)   — render to the display buffer.
//               The OS calls clearBuffer() before and sendBuffer() after;
//               games never call either themselves.
//
// Both methods receive a Console& — the single context object that wraps
// the display, input, and sound systems.  Games never include U8g2lib.h,
// InputManager.h, or Sound.h directly.
//
// To exit back to the menu: set your internal _running flag to false.
// The OS detects it via isRunning() and calls onExit() before returning.
// ─────────────────────────────────────────────────────────────────────────────
class GameBase {
public:
    virtual ~GameBase() = default;

    // Called once when the game is launched from the menu.
    virtual void onEnter() = 0;

    // Called once when the game exits back to the menu.
    virtual void onExit()  = 0;

    // Called every frame: advance game logic, read input, trigger sounds.
    virtual void update(Console& ctx) = 0;

    // Called every frame after update: render the current frame.
    virtual void draw(Console& ctx)   = 0;

    // Returns false when the game wants to exit back to the menu.
    virtual bool isRunning() const = 0;

    // Short display name shown in the OS menu (max ~12 chars).
    virtual const char* getName() const = 0;

    // Optional 16×16 PROGMEM icon bitmap for the menu card.
    // Return nullptr to use the OS placeholder (initial letter in a box).
    virtual const uint8_t* getIcon() const { return nullptr; }
};