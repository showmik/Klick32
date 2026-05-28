#include "SynthEngine.h"

#ifdef SIMULATOR
#include <SDL.h>
#endif

uint8_t SynthEngine::_pin = 0;
bool SynthEngine::_muted = false;
Voice SynthEngine::_voices[4];
TaskHandle_t SynthEngine::_taskHandle = nullptr;

void SynthEngine::begin(uint8_t pin) {
    _pin = pin;

#ifdef SIMULATOR
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_S8;
    want.channels = 1;
    want.samples = 512;
    want.callback = _audioTask;
    
    SDL_AudioDeviceID dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (dev != 0) {
        SDL_PauseAudioDevice(dev, 0);
    }
#else
    // ESP32 Arduino Core 3.x API for LEDC
    ledcAttach(_pin, 100000, 8); // pin, freq, resolution
    ledcWrite(_pin, 0);

    // Create FreeRTOS task pinned to Core 0 for audio mixing
    xTaskCreatePinnedToCore(
        _audioTask,
        "AudioMixerTask",
        4096, // Stack size
        nullptr,
        configMAX_PRIORITIES - 1, // High priority
        &_taskHandle,
        0 // Core 0
    );
#endif
}

int SynthEngine::playTone(uint16_t freqHz, uint32_t durationMs, Waveform wave) {
    if (_muted) return -1;

    // Find a free voice, or the oldest active voice to steal
    int bestVoice = 0;
    uint32_t oldestTime = 0xFFFFFFFF;
    
    for (int i = 0; i < 4; i++) {
        if (!_voices[i].active) {
            bestVoice = i;
            break;
        }
        if (_voices[i].startTimeMs < oldestTime) {
            oldestTime = _voices[i].startTimeMs;
            bestVoice = i;
        }
    }

    Voice& v = _voices[bestVoice];
    v.freqHz = freqHz;
    v.durationMs = durationMs;
    v.wave = wave;
    v.startTimeMs = millis();
    v.phase = 0.0f;
    v.phaseInc = (float)freqHz / (float)SAMPLE_RATE;
    v.active = true;
    
    // Default envelope based on waveform
    v.attackMs = (wave == Waveform::NOISE) ? 1 : 10;
    v.releaseMs = (wave == Waveform::NOISE) ? durationMs / 2 : 50;

    return bestVoice;
}

void SynthEngine::stopAll() {
    for (int i = 0; i < 4; i++) {
        _voices[i].active = false;
    }
#ifndef SIMULATOR
    ledcWrite(_pin, 0);
#endif
}

void SynthEngine::setMuted(bool m) {
    _muted = m;
    if (m) stopAll();
}

bool SynthEngine::isMuted() {
    return _muted;
}

#ifdef SIMULATOR
// In SDL mode, this is a callback function requested periodically by SDL
void SynthEngine::_audioTask(void* userdata, uint8_t* stream, int len) {
    if (_muted) {
        memset(stream, 0, len);
        return;
    }

    uint32_t nowMs = millis();
    for (int i = 0; i < len; i++) {
        int32_t mixedSample = 0;
        int activeCount = 0;

        for (int vIdx = 0; vIdx < 4; vIdx++) {
            Voice& v = _voices[vIdx];
            if (!v.active) continue;

            if (nowMs - v.startTimeMs > v.durationMs) {
                v.active = false;
                continue;
            }

            activeCount++;
            v.phase += v.phaseInc;
            if (v.phase >= 1.0f) v.phase -= 1.0f;

            float sample = 0.0f;
            if (v.wave == Waveform::SQUARE) {
                sample = (v.phase < 0.5f) ? 1.0f : -1.0f;
            } else if (v.wave == Waveform::TRIANGLE) {
                sample = (v.phase < 0.5f) ? (v.phase * 4.0f - 1.0f) : (3.0f - v.phase * 4.0f);
            } else if (v.wave == Waveform::NOISE) {
                sample = ((float)random(0, 1000) / 500.0f) - 1.0f;
            }
            
            // Apply Attack / Release envelope
            uint32_t elapsed = nowMs - v.startTimeMs;
            float env = 1.0f;
            if (elapsed < v.attackMs) {
                env = (float)elapsed / (float)v.attackMs;
            } else if (elapsed > v.durationMs - v.releaseMs) {
                env = (float)(v.durationMs - elapsed) / (float)v.releaseMs;
            }
            
            mixedSample += (int32_t)(sample * env * 127.0f);
        }

        if (activeCount > 0) {
            int32_t outSample = (mixedSample / activeCount);
            if (outSample < -128) outSample = -128;
            if (outSample > 127) outSample = 127;
            stream[i] = (int8_t)outSample;
        } else {
            stream[i] = 0;
        }
    }
}
#else
// ESP32 FreeRTOS loop
void SynthEngine::_audioTask(void* pvParameters) {
    const uint32_t delayUs = 1000000 / SAMPLE_RATE;
    uint32_t lastWakeTime = esp_timer_get_time();

    while (true) {
        if (_muted) {
            vTaskDelay(pdMS_TO_TICKS(10));
            lastWakeTime = esp_timer_get_time();
            continue;
        }

        uint32_t nowMs = millis();
        int32_t mixedSample = 0;
        int activeCount = 0;

        for (int i = 0; i < 4; i++) {
            Voice& v = _voices[i];
            if (!v.active) continue;

            if (nowMs - v.startTimeMs > v.durationMs) {
                v.active = false;
                continue;
            }

            activeCount++;
            v.phase += v.phaseInc;
            if (v.phase >= 1.0f) v.phase -= 1.0f;

            float sample = 0.0f;
            if (v.wave == Waveform::SQUARE) {
                sample = (v.phase < 0.5f) ? 1.0f : -1.0f;
            } else if (v.wave == Waveform::TRIANGLE) {
                sample = (v.phase < 0.5f) ? (v.phase * 4.0f - 1.0f) : (3.0f - v.phase * 4.0f);
            } else if (v.wave == Waveform::NOISE) {
                sample = ((float)random(0, 1000) / 500.0f) - 1.0f;
            }
            
            // Apply Attack / Release envelope
            uint32_t elapsed = nowMs - v.startTimeMs;
            float env = 1.0f;
            if (v.attackMs > 0 && elapsed < v.attackMs) {
                env = (float)elapsed / (float)v.attackMs;
            } else if (v.releaseMs > 0 && v.durationMs >= v.releaseMs && elapsed > (v.durationMs - v.releaseMs)) {
                env = (float)(v.durationMs - elapsed) / (float)v.releaseMs;
            }
            
            mixedSample += (int32_t)(sample * env * 127.0f);
        }

        if (activeCount > 0) {
            // Average the voices to prevent clipping, convert back to 0-255 duty cycle
            int32_t outSample = (mixedSample / activeCount) + 128;
            if (outSample < 0) outSample = 0;
            if (outSample > 255) outSample = 255;
            ledcWrite(_pin, (uint8_t)outSample);
        } else {
            ledcWrite(_pin, 0); // Silence
        }

        // Wait until next sample period (using esp_timer for precision)
        uint32_t nowUs = esp_timer_get_time();
        uint32_t elapsed = nowUs - lastWakeTime;
        if (elapsed < delayUs) {
            // Microsecond delay is fine here for high-frequency polling, but must yield periodically to Watchdog
            esp_rom_delay_us(delayUs - elapsed);
        }
        lastWakeTime = esp_timer_get_time();

        // Feed watchdog periodically (every ~10ms / 220 samples)
        static int yieldCounter = 0;
        if (++yieldCounter >= 220) {
            yieldCounter = 0;
            vTaskDelay(1);
        }
    }
}
#endif
