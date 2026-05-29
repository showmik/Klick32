#pragma once
#include <Arduino.h>
#include "Config.h"

// ─── Battery ──────────────────────────────────────────────────────────────────
// Reads battery level via ADC using a 100kΩ / 100kΩ voltage divider.
//
// Wiring:
//   VBAT ──[ 100k ]──┬──[ 100k ]── GND
//                    └── PIN_BATT_ADC
//
// PIN_CHRG connects to the CHRG pin of your charging IC (TP4056, MCP73831, etc.)
// It reads LOW while actively charging, HIGH when full or disconnected.
// If you have no CHRG pin, leave PIN_CHRG tied to 3.3V — isCharging() returns false.
class Battery {
public:
    void begin() {
#ifndef SIMULATOR
        analogReadResolution(12); // 12-bit ADC (0–4095)
        pinMode(PIN_CHRG, INPUT_PULLUP);
#endif
    }

    // Returns 0–100. Uses a rolling average of 8 samples to reduce ADC noise.
    // Non-blocking: takes one new ADC reading per call and feeds it into the
    // ring buffer. Call once every few seconds, not every frame.
    uint8_t readPercent() {
#ifndef SIMULATOR
        uint32_t sample = analogReadMilliVolts(PIN_BATT_ADC);

        // First call: fill the entire ring buffer to avoid a ramp-up period
        if (!_bufferFilled) {
            for (uint8_t i = 0; i < SAMPLE_COUNT; i++) _samples[i] = sample;
            _bufferFilled = true;
        } else {
            _samples[_sampleIdx] = sample;
        }
        _sampleIdx = (_sampleIdx + 1) % SAMPLE_COUNT;

        // Average all samples
        uint32_t sum = 0;
        for (uint8_t i = 0; i < SAMPLE_COUNT; i++) sum += _samples[i];
        uint32_t vadc_mv = sum / SAMPLE_COUNT;
        uint32_t vbat_mv = vadc_mv * 2; // undo the 1:2 voltage divider

        // LiPo range: 3300 mV (empty) → 4200 mV (full)
        const uint32_t VMIN = 3300;
        const uint32_t VMAX = 4200;

        if (vbat_mv <= VMIN) return 0;
        if (vbat_mv >= VMAX) return 100;
        return (uint8_t)((vbat_mv - VMIN) * 100UL / (VMAX - VMIN));
#else
        return 100;
#endif
    }

    // True when charging IC asserts CHRG LOW.
    bool isCharging() const {
#ifndef SIMULATOR
        return digitalRead(PIN_CHRG) == LOW;
#else
        return false;
#endif
    }

private:
    static constexpr uint8_t SAMPLE_COUNT = 8;
    uint32_t _samples[SAMPLE_COUNT] = {};
    uint8_t  _sampleIdx    = 0;
    bool     _bufferFilled = false;
};