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
// Call begin() once in setup(), update() once at the top of every frame.
// All buttons are active-LOW (INPUT_PULLUP).
//
// ── Debounce ─────────────────────────────────────────────────────────────────
// A state change is only committed once the pin has been stable for
// DEBOUNCE_FRAMES consecutive frames (~33 ms at 30 fps). This filters
// mechanical bounce without adding noticeable input lag.
//
// justPressed() / justReleased() fire on a single frame edge, so game
// logic sees at most one event per physical button press.
//
// ── Repeat ───────────────────────────────────────────────────────────────────
// repeat() is intended for UI navigation (menu scroll, cursor movement).
// It fires on the same frame as justPressed(), then again after
// REPEAT_DELAY frames of holding, then every REPEAT_RATE frames.
//
//   Timeline (DELAY=20, RATE=6):
//   Frame:  0  1 … 20  26  32  38 …
//   Fire:   ✓          ✓   ✓   ✓
//
// Use justPressed() for physics actions (jump, shoot) — one event per press.
// Use repeat()      for navigation — scrolls while held.
//
// ── holdFrames() ─────────────────────────────────────────────────────────────
// Returns consecutive frames in the pressed state; resets to 0 on release.
// Useful for long-press actions, e.g.:
//   if (input.holdFrames(Btn::MENU1) == 60) resetHiScore();

class InputManager {
public:
    static constexpr uint8_t NUM_BTNS = (uint8_t)Btn::COUNT;

    // ── Tuning constants ──────────────────────────────────────────────────────
    static constexpr uint8_t DEBOUNCE_FRAMES = 1;   // frames pin must be stable
    static constexpr uint8_t REPEAT_DELAY    = 20;  // frames before first repeat  (~660 ms)
    static constexpr uint8_t REPEAT_RATE     = 6;   // frames between repeats      (~200 ms)

    // ─────────────────────────────────────────────────────────────────────────

    void begin() {
        for (uint8_t i = 0; i < NUM_BTNS; i++) {
            pinMode(_pins[i], INPUT_PULLUP);
            _s[i] = {};
        }
    }

    // Sample all pins. Call exactly once at the start of each frame.
    void update() {
        for (uint8_t i = 0; i < NUM_BTNS; i++) {
            BtnState& s = _s[i];

            bool raw = (digitalRead(_pins[i]) == LOW);  // LOW = pressed (active-LOW)

            // ── Debounce ──────────────────────────────────────────────────────
            if (raw == s.debounced) {
                s.noiseFrames = 0;                      // stable — reset noise counter
            } else {
                if (++s.noiseFrames >= DEBOUNCE_FRAMES) {
                    s.noiseFrames = 0;
                    s.debounced   = raw;                // commit the state change
                }
            }

            // ── Edge detection ────────────────────────────────────────────────
            s.justPressed_  = ( s.debounced && !s.prev);
            s.justReleased_ = (!s.debounced &&  s.prev);
            s.prev          =  s.debounced;

            // ── Hold counter ──────────────────────────────────────────────────
            s.holdFrames = s.debounced
                           ? (s.holdFrames < 0xFFFFu ? s.holdFrames + 1u : 0xFFFFu)
                           : 0u;

            // ── Repeat ────────────────────────────────────────────────────────
            if (s.justPressed_) {
                s.repeat_ = true;                       // always fire on initial press
            } else if (s.debounced && s.holdFrames >= REPEAT_DELAY) {
                uint16_t elapsed = s.holdFrames - REPEAT_DELAY;
                s.repeat_ = (elapsed % REPEAT_RATE == 0);
            } else {
                s.repeat_ = false;
            }
        }
    }

    // ── Queries ───────────────────────────────────────────────────────────────

    /// True every frame the button is held (debounced).
    bool held(Btn b) const {
        return _s[(uint8_t)b].debounced;
    }

    /// True on the ONE frame the button goes released → pressed.
    /// Use for one-shot actions: jump, shoot, menu confirm.
    bool justPressed(Btn b) const {
        return _s[(uint8_t)b].justPressed_;
    }

    /// True on the ONE frame the button goes pressed → released.
    bool justReleased(Btn b) const {
        return _s[(uint8_t)b].justReleased_;
    }

    /// True on justPressed AND periodically while held.
    /// Use for UI navigation: menu scroll, cursor movement.
    bool repeat(Btn b) const {
        return _s[(uint8_t)b].repeat_;
    }

    /// Frames the button has been continuously held (debounced).
    /// Resets to 0 on release.
    uint16_t holdFrames(Btn b) const {
        return _s[(uint8_t)b].holdFrames;
    }

private:
    struct BtnState {
        bool     debounced     = false;
        bool     prev          = false;
        bool     justPressed_  = false;
        bool     justReleased_ = false;
        bool     repeat_       = false;
        uint8_t  noiseFrames   = 0;
        uint16_t holdFrames    = 0;
    };

    const uint8_t _pins[NUM_BTNS] = {
        PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT,
        PIN_BTN_A, PIN_BTN_B, PIN_MENU1, PIN_MENU2
    };

    BtnState _s[NUM_BTNS];
};