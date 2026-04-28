#pragma once
#include <U8g2lib.h>
#include "InputManager.h"
#include "Sound.h"

// ─── GameBase ─────────────────────────────────────────────────────────────────
// Every game inherits from this class and implements all pure-virtual methods.
//
// Lifecycle managed by the OS:
//   1. OS calls onEnter()  when the game is launched from the menu.
//   2. OS calls update() + draw() every frame while the game is active.
//   3. When the game sets its internal running flag to false,
//      isRunning() returns false and the OS calls onExit() before
//      returning to the menu.
//
// Frame contract:
//   - update() : update logic, read input, trigger sounds.
//   - draw()   : render to the U8G2 buffer.
//                OS calls clearBuffer() before and sendBuffer() after.
//
// To exit back to the menu: press MENU1 (or any game-defined back button)
// and set your internal _running flag to false.
class GameBase {
public:
    virtual ~GameBase() = default;

    virtual void onEnter() = 0;
    virtual void onExit()  = 0;

    virtual void update(InputManager& input, Sound& sound) = 0;
    virtual void draw(U8G2& disp) = 0;

    virtual bool isRunning() const = 0;

    // Short display name shown in the OS menu (max ~12 chars).
    virtual const char* getName() const = 0;

    // Optional 16×16 PROGMEM icon bitmap.
    // Return nullptr to use the OS default placeholder (initial letter in a box).
    virtual const uint8_t* getIcon() const { return nullptr; }
};