#pragma once
#include <math.h>

// ─── Easing ───────────────────────────────────────────────────────────────────
// Pure math easing functions. Zero state, zero allocation.
// All functions take t in [0.0, 1.0] and return a curved value.
//
// Usage with lerpf():
//   float t = timer.progress();
//   float x = lerpf(startX, endX, Ease::outQuad(t));
//
// Usage with lerpi():
//   int x = lerpi(0, 128, (int)(Ease::outBounce(t) * 10), 10);
//
// Function pointer type for passing easing to other systems:
//   typedef float (*EaseFunc)(float);
// ─────────────────────────────────────────────────────────────────────────────

typedef float (*EaseFunc)(float);

namespace Ease {

    // ── Linear ───────────────────────────────────────────────────────────────
    inline float linear(float t) { return t; }

    // ── Quadratic ────────────────────────────────────────────────────────────
    inline float inQuad(float t)    { return t * t; }
    inline float outQuad(float t)   { return t * (2.0f - t); }
    inline float inOutQuad(float t) {
        return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
    }

    // ── Cubic ────────────────────────────────────────────────────────────────
    inline float inCubic(float t)    { return t * t * t; }
    inline float outCubic(float t)   { float u = t - 1.0f; return u * u * u + 1.0f; }
    inline float inOutCubic(float t) {
        return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
    }

    // ── Quartic ──────────────────────────────────────────────────────────────
    inline float inQuart(float t)  { return t * t * t * t; }
    inline float outQuart(float t) { float u = t - 1.0f; return 1.0f - u * u * u * u; }

    // ── Sine ─────────────────────────────────────────────────────────────────
    inline float inSine(float t)    { return 1.0f - cosf(t * M_PI / 2.0f); }
    inline float outSine(float t)   { return sinf(t * M_PI / 2.0f); }
    inline float inOutSine(float t) { return -(cosf(M_PI * t) - 1.0f) / 2.0f; }

    // ── Bounce ───────────────────────────────────────────────────────────────
    inline float outBounce(float t) {
        if (t < 1.0f / 2.75f) {
            return 7.5625f * t * t;
        } else if (t < 2.0f / 2.75f) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        } else if (t < 2.5f / 2.75f) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        } else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }
    inline float inBounce(float t) { return 1.0f - outBounce(1.0f - t); }

    // ── Elastic ──────────────────────────────────────────────────────────────
    // NOTE: overshoots past 1.0 — intentional for springy effects.
    inline float outElastic(float t) {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        return powf(2.0f, -10.0f * t) * sinf((t - 0.075f) * (2.0f * M_PI) / 0.3f) + 1.0f;
    }
    inline float inElastic(float t) { return 1.0f - outElastic(1.0f - t); }

    // ── Back ─────────────────────────────────────────────────────────────────
    // NOTE: overshoots slightly — intentional for anticipation/overshoot.
    inline float inBack(float t) {
        constexpr float s = 1.70158f;
        return t * t * ((s + 1.0f) * t - s);
    }
    inline float outBack(float t) {
        constexpr float s = 1.70158f;
        float u = t - 1.0f;
        return u * u * ((s + 1.0f) * u + s) + 1.0f;
    }

} // namespace Ease
