#pragma once
#include <Arduino.h>
#include "Config.h"

// ─── Sound ────────────────────────────────────────────────────────────────────
// Thin wrapper around tone(). Non-blocking — uses ESP32 LEDC timer internally.
class Sound {
public:
    void begin() {
        pinMode(PIN_BUZZER, OUTPUT);
        digitalWrite(PIN_BUZZER, LOW);
    }

    // Play a tone at freqHz for durationMs milliseconds (non-blocking).
    void beep(uint16_t freqHz, uint32_t durationMs = 50) {
        if (_muted) return;
        tone(PIN_BUZZER, freqHz, durationMs);
    }

    void stop() { noTone(PIN_BUZZER); }

    void setMuted(bool m) { _muted = m; if (m) stop(); }
    void toggleMute()     { setMuted(!_muted); }
    bool isMuted()  const { return _muted; }

private:
    bool _muted = false;
};

// ─── SFX ──────────────────────────────────────────────────────────────────────
// Predefined sound effects shared across the OS and all games.
namespace SFX {
    inline void menuNav(Sound& s)   { s.beep( 800,  30); }
    inline void menuEnter(Sound& s) { s.beep(1200,  70); }
    inline void menuBack(Sound& s)  { s.beep( 500,  60); }
    inline void jump(Sound& s)      { s.beep( 520,  50); }
    inline void death(Sound& s)     { s.beep( 200, 350); }
    inline void point(Sound& s)     { s.beep(1000,  20); }
    inline void unmute(Sound& s)    { s.beep( 800,  40); }
}