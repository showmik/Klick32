#pragma once
#include <Arduino.h>
#include "Config.h"
#include "SynthEngine.h"

// ─── Sound ────────────────────────────────────────────────────────────────────
// Wrapper around the new polyphonic SynthEngine.
class Sound {
public:
    void begin() {
        SynthEngine::begin(PIN_BUZZER);
    }

    // Play a tone at freqHz for durationMs milliseconds (non-blocking).
    void beep(uint16_t freqHz, uint32_t durationMs = 50) {
        SynthEngine::playTone(freqHz, durationMs, Waveform::SQUARE);
    }
    
    // Play a noise burst (explosions, hits)
    void noise(uint32_t durationMs = 50) {
        SynthEngine::playTone(440, durationMs, Waveform::NOISE);
    }

    void stop() { SynthEngine::stopAll(); }

    void setMuted(bool m) { SynthEngine::setMuted(m); }
    void toggleMute()     { SynthEngine::setMuted(!SynthEngine::isMuted()); }
    bool isMuted()  const { return SynthEngine::isMuted(); }
};

// ─── SFX ──────────────────────────────────────────────────────────────────────
// Predefined sound effects shared across the OS and all games.
namespace SFX {
    inline void menuNav(Sound& s)   { s.beep( 800,  30); }
    inline void menuEnter(Sound& s) { s.beep(1200,  70); }
    inline void menuBack(Sound& s)  { s.beep( 500,  60); }
    inline void jump(Sound& s)      { s.beep( 520,  50); }
    inline void death(Sound& s)     { s.noise(350); }
    inline void point(Sound& s)     { s.beep(1000,  20); }
    inline void unmute(Sound& s)    { s.beep( 800,  40); }
}