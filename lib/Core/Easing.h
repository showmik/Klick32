#pragma once

// ─── Easing ──────────────────────────────────────────────────────────────────
// Collection of standard easing functions.
// All functions take a progress parameter t in [0.0, 1.0] and return
// an eased value typically in [0.0, 1.0] (some, like elastic/bounce, may exceed).
// ─────────────────────────────────────────────────────────────────────────────
namespace Ease {
    inline float linear(float t) { return t; }
    
    inline float inQuad(float t) { return t * t; }
    inline float outQuad(float t) { return t * (2.0f - t); }
    inline float inOutQuad(float t) { return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t; }
    
    inline float inCubic(float t) { return t * t * t; }
    inline float outCubic(float t) { float f = t - 1.0f; return f * f * f + 1.0f; }
    inline float inOutCubic(float t) { return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f; }
    
    inline float outBounce(float t) {
        if (t < (1.0f / 2.75f)) {
            return 7.5625f * t * t;
        } else if (t < (2.0f / 2.75f)) {
            float f = t - (1.5f / 2.75f);
            return 7.5625f * f * f + 0.75f;
        } else if (t < (2.5f / 2.75f)) {
            float f = t - (2.25f / 2.75f);
            return 7.5625f * f * f + 0.9375f;
        } else {
            float f = t - (2.625f / 2.75f);
            return 7.5625f * f * f + 0.984375f;
        }
    }
    
    inline float outElastic(float t) {
        if (t == 0.0f || t == 1.0f) return t;
        float p = 0.3f;
        return powf(2.0f, -10.0f * t) * sinf((t - p / 4.0f) * (2.0f * 3.14159265f) / p) + 1.0f;
    }
}
