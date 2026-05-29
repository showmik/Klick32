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
// ── Press vs Release debounce strategy ───────────────────────────────────────
//
// PRESS   → registered on the FIRST LOW reading, zero delay.
//           A tap as short as one polling cycle (~33 ms at 30 fps) is caught.
//
// RELEASE → committed only after the pin has been HIGH for RELEASE_FRAMES
//           consecutive frames (~66 ms). This absorbs contact bounce on
//           release without adding any lag to the press itself.
//
// This asymmetric approach gives maximum responsiveness for game actions
// (jump, shoot) while still filtering the noise that matters most.
//
// ── Repeat ───────────────────────────────────────────────────────────────────
// repeat() fires on justPressed, then after REPEAT_DELAY frames of holding,
// then every REPEAT_RATE frames. Intended for UI navigation, not game physics.
//
// ── holdFrames() ─────────────────────────────────────────────────────────────
// Counts consecutive pressed frames; resets to 0 on release.
// Useful for long-press actions:
//   if (input.holdFrames(Btn::MENU1) == 60) resetHiScore();

class InputManager {
public:
    static constexpr uint8_t NUM_BTNS = (uint8_t)Btn::COUNT;

    // ── Tuning ────────────────────────────────────────────────────────────────
    static constexpr uint8_t RELEASE_FRAMES = 2;   // frames HIGH before release commits (~66 ms)
    static constexpr uint8_t REPEAT_DELAY   = 12;  // Reduced from 20 -> ~396 ms before fast scroll triggers
    static constexpr uint8_t REPEAT_RATE    = 3;   // Reduced from 6 -> ~99 ms intervals during held scrolling

    void begin() {
        for (uint8_t i = 0; i < NUM_BTNS; i++) {
            pinMode(_pins[i], INPUT_PULLUP);
            _s[i] = {};
        }
    }

    // Call exactly once per frame before any input queries.
    void update() {
        for (uint8_t i = 0; i < NUM_BTNS; i++) {
            BtnState& s = _s[i];

            bool raw = (digitalRead(_pins[i]) == LOW);  // LOW = pressed

            // ── Asymmetric debounce ───────────────────────────────────────────
            if (raw) {
                // Pin is LOW → pressed: commit immediately, reset release counter
                s.debounced   = true;
                s.releaseFrames = 0;
            } else {
                // Pin is HIGH → might be released or just bouncing
                if (s.debounced) {
                    // Was pressed — wait for RELEASE_FRAMES of stability
                    if (++s.releaseFrames >= RELEASE_FRAMES) {
                        s.debounced = false;
                    }
                }
                // If already not debounced, nothing to do
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
                s.repeat_ = true;
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
    /// Use for UI navigation — menu scroll, cursor move.
    bool repeat(Btn b) const {
        return _s[(uint8_t)b].repeat_;
    }

    /// Consecutive frames the button has been held. Resets to 0 on release.
    uint16_t holdFrames(Btn b) const {
        return _s[(uint8_t)b].holdFrames;
    }

private:
    struct BtnState {
        bool     debounced      = false;
        bool     prev           = false;
        bool     justPressed_   = false;
        bool     justReleased_  = false;
        bool     repeat_        = false;
        uint8_t  releaseFrames  = 0;
        uint16_t holdFrames     = 0;
    };

    const uint8_t _pins[NUM_BTNS] = {
        PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT,
        PIN_BTN_A, PIN_BTN_B, PIN_MENU1, PIN_MENU2
    };

    BtnState _s[NUM_BTNS];
};