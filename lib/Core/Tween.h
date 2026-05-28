#pragma once
#include "Timer.h"
#include "Easing.h"

// ─── Tween ───────────────────────────────────────────────────────────────────
// A lightweight tween value wrapper. 
// Allows easy animation of values over time using easing functions.
//
// Usage:
//   Tween<int> x;
//   x.start(0, 100, 500, Ease::outQuad);
//   // In update:
//   draw(x.val(), y);
// ─────────────────────────────────────────────────────────────────────────────

template<typename T>
class Tween {
public:
    Tween() : _val(T()), _startVal(T()), _endVal(T()), _ease(Ease::linear) {}
    Tween(T initialVal) : _val(initialVal), _startVal(initialVal), _endVal(initialVal), _ease(Ease::linear) {}

    // Start a new tween animation
    void start(T from, T to, uint32_t durationMs, float (*easeFunc)(float) = Ease::linear) {
        _startVal = from;
        _endVal = to;
        _val = from;
        _ease = easeFunc;
        _timer.start(durationMs);
    }

    // Call this if you want to poll if it just finished, though not strictly required
    bool update() {
        if (_timer.isRunning()) {
            float t = _timer.progress();
            if (t >= 1.0f) {
                _timer.stop();
                _val = _endVal;
                return true; // just finished
            } else {
                float e = _ease(t);
                _val = _startVal + (_endVal - _startVal) * e;
            }
        }
        return false;
    }

    // Get current value (auto-updates if timer is running)
    T val() {
        update();
        return _val;
    }

    // Implicit cast to T
    operator T() { return val(); }

    bool isRunning() const { return _timer.isRunning(); }

private:
    T _val;
    T _startVal;
    T _endVal;
    Timer _timer;
    float (*_ease)(float);
};
