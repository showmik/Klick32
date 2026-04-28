#pragma once
#include <Arduino.h>
#include "Config.h"

// ─── Button Identifiers ───────────────────────────────────────────────────────
enum class Btn : uint8_t {
    UP = 0, DOWN, LEFT, RIGHT,
    A, B,
    MENU1, MENU2,
    COUNT
};

// ─── InputManager ─────────────────────────────────────────────────────────────
// Call begin() once in setup, update() once per frame.
// All buttons are active-LOW (INPUT_PULLUP).
class InputManager {
public:
    static constexpr uint8_t NUM_BTNS = (uint8_t)Btn::COUNT;

    void begin() {
        for (uint8_t i = 0; i < NUM_BTNS; i++) {
            pinMode(_pins[i], INPUT_PULLUP);
            _cur[i] = _prev[i] = true; // HIGH = released
        }
    }

    // Sample all pins. Must be called once at the start of each frame.
    void update() {
        for (uint8_t i = 0; i < NUM_BTNS; i++) {
            _prev[i] = _cur[i];
            _cur[i]  = (digitalRead(_pins[i]) == HIGH);
        }
    }

    // True while the button is physically held down.
    bool held(Btn b) const {
        return !_cur[(uint8_t)b];
    }

    // True on the single frame the button goes from released → pressed.
    bool justPressed(Btn b) const {
        uint8_t i = (uint8_t)b;
        return (!_cur[i] && _prev[i]);
    }

    // True on the single frame the button goes from pressed → released.
    bool justReleased(Btn b) const {
        uint8_t i = (uint8_t)b;
        return (_cur[i] && !_prev[i]);
    }

private:
    const uint8_t _pins[NUM_BTNS] = {
        PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT,
        PIN_BTN_A, PIN_BTN_B, PIN_MENU1, PIN_MENU2
    };
    bool _cur[NUM_BTNS]  = {};
    bool _prev[NUM_BTNS] = {};
};