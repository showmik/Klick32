#pragma once
#include <Arduino.h>
#include "GameUtils.h"

// ─── Timer ────────────────────────────────────────────────────────────────────
// Lightweight timer to replace raw millis() tracking.
// No dynamic allocation. Call tick() once per frame.
//
// One-shot usage:
//   Timer invincible;
//   invincible.start(2000);           // 2 second invincibility
//   if (invincible.isRunning()) { /* flash sprite */ }
//   if (invincible.tick()) { /* invincibility wore off */ }
//
// Repeating usage:
//   Timer spawner;
//   spawner.startRepeating(500);      // spawn every 500ms
//   if (spawner.tick()) { spawnEnemy(); }
//
// Progress (for animations):
//   Timer fadeIn;
//   fadeIn.start(300);
//   float alpha = fadeIn.progress();  // 0.0 → 1.0 over 300ms
// ─────────────────────────────────────────────────────────────────────────────

class Timer {
public:
    Timer() = default;

    // Start a one-shot timer that fires once after durationMs.
    void start(uint32_t durationMs) {
        _startTime = millis();
        _duration = durationMs;
        _repeating = false;
        _running = true;
    }

    // Start a repeating timer that fires every intervalMs.
    void startRepeating(uint32_t intervalMs) {
        _startTime = millis();
        _duration = intervalMs;
        _repeating = true;
        _running = true;
    }

    // Call once per frame. Returns true when the timer fires.
    // For one-shot timers, stops after firing once.
    // For repeating timers, resets and continues.
    bool tick() {
        if (!_running) return false;

        uint32_t now = millis();
        if (now - _startTime >= _duration) {
            if (_repeating) {
                _startTime += _duration; // drift-free reset
                // If we've fallen behind by more than one interval, reset to now
                if (now - _startTime >= _duration) _startTime = now;
            } else {
                _running = false;
            }
            return true;
        }
        return false;
    }

    // True if the timer is currently counting.
    bool isRunning() const { return _running; }

    // Returns how far through the timer we are: 0.0 (just started) → 1.0 (about to fire).
    // Returns 1.0 if not running (already fired or never started).
    float progress() const {
        if (!_running || _duration == 0) return 1.0f;
        uint32_t elapsed = millis() - _startTime;
        return gclamp((float)elapsed / (float)_duration, 0.0f, 1.0f);
    }

    // Milliseconds remaining. Returns 0 if not running.
    uint32_t remaining() const {
        if (!_running) return 0;
        uint32_t elapsed = millis() - _startTime;
        return elapsed >= _duration ? 0 : _duration - elapsed;
    }

    // Stop the timer without firing.
    void stop() { _running = false; }

    // Restart from the current moment (same duration as last start call).
    void restart() {
        _startTime = millis();
        _running = true;
    }

private:
    uint32_t _startTime = 0;
    uint32_t _duration  = 0;
    bool     _repeating = false;
    bool     _running   = false;
};
