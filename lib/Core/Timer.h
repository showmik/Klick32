#pragma once
#include <Arduino.h>

// ─── Timer ───────────────────────────────────────────────────────────────────
// A lightweight utility for tracking elapsed time.
// Replaces manual `millis()` tracking in games.
// ─────────────────────────────────────────────────────────────────────────────
class Timer {
public:
    Timer() = default;

    // Start a one-shot timer for the given duration in milliseconds
    void start(uint32_t durationMs) {
        _duration = durationMs;
        _startTime = millis();
        _running = true;
        _repeating = false;
    }

    // Start a repeating timer (ticks continuously)
    void startRepeating(uint32_t intervalMs) {
        _duration = intervalMs;
        _startTime = millis();
        _running = true;
        _repeating = true;
    }

    // Stop the timer
    void stop() {
        _running = false;
    }

    // Check if the timer is currently running
    bool isRunning() const {
        return _running;
    }

    // Check if the timer has elapsed. If repeating, automatically resets for the next tick.
    bool tick() {
        if (!_running) return false;
        if (millis() - _startTime >= _duration) {
            if (_repeating) {
                // Keep the interval precise by adding duration, rather than resetting to current millis()
                _startTime += _duration;
            } else {
                _running = false;
            }
            return true;
        }
        return false;
    }

    // Get progress from 0.0f to 1.0f
    float progress() const {
        if (!_running) return 1.0f;
        uint32_t elapsed = millis() - _startTime;
        if (elapsed >= _duration) return 1.0f;
        return (float)elapsed / (float)_duration;
    }

private:
    uint32_t _startTime = 0;
    uint32_t _duration = 0;
    bool _running = false;
    bool _repeating = false;
};
