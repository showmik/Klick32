#pragma once
#include <Arduino.h>

// ─── SynthEngine ──────────────────────────────────────────────────────────────
// A 4-voice software synthesizer running asynchronously on Core 0.
// Generates polyphonic audio (Square, Triangle, Noise) and outputs via LEDC PWM.
// ─────────────────────────────────────────────────────────────────────────────

enum class Waveform : uint8_t { SQUARE, TRIANGLE, NOISE };

struct Voice {
    bool active = false;
    Waveform wave = Waveform::SQUARE;
    uint32_t freqHz = 0;
    uint32_t durationMs = 0;
    uint32_t startTimeMs = 0;
    float phase = 0.0f;
    float phaseInc = 0.0f;
    
    // Envelope (Attack/Release in ms)
    uint16_t attackMs = 5;
    uint16_t releaseMs = 50;
};

class SynthEngine {
public:
    static void begin(uint8_t pin);
    
    // Automatically selects a free voice (or steals the oldest). Returns the voice index.
    static int playTone(uint16_t freqHz, uint32_t durationMs, Waveform wave = Waveform::SQUARE);
    
    static void stopAll();
    static void setMuted(bool m);
    static bool isMuted();

private:
#ifdef SIMULATOR
    static void _audioTask(void* userdata, uint8_t* stream, int len);
#else
    static void _audioTask(void* pvParameters);
#endif
    
    static uint8_t _pin;
    static bool _muted;
    static Voice _voices[4];
    static TaskHandle_t _taskHandle;
    
    // Sample rate for the software mixer (e.g., 22050 Hz)
    static constexpr uint32_t SAMPLE_RATE = 22050;
};
