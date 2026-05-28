#pragma once
#include <Arduino.h>

// ─── GameUtils.h ──────────────────────────────────────────────────────────────
// Lightweight, zero-overhead utilities shared across all games.
//
// Contents:
//   Vec2  — float 2-D vector for positions, velocities, and offsets.
//   Rect  — integer AABB for collision hitboxes.
//   gclamp / lerpi / gsign / wrapIdx — common math helpers.
//
// Design rules:
//   • No dynamic allocation. No virtual calls. Header-only.
//   • Every function is inline so the linker emits only what is used.
//   • Helpers are prefixed (g-) to avoid collisions with Arduino / C++ stdlib.
// ─────────────────────────────────────────────────────────────────────────────

// Clamp v into the closed interval [lo, hi].
// Prefixed 'g' to avoid conflict with std::clamp (C++17).
//
//   float speed = gclamp(speed + inc, 0.0f, MAX_SPEED);
template<typename T>
inline T gclamp(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}


// ─── Vec2 ────────────────────────────────────────────────────────────────────
// General-purpose float vector.
// Use for anything that moves: sprite positions, velocities, cloud offsets.
//
// Example:
//   Vec2 pos {10.0f, 36.0f};
//   Vec2 vel { 0.0f, -8.0f};
//   pos += vel;
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    // ── Arithmetic ────────────────────────────────────────────────────────────
    Vec2  operator+ (const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2  operator- (const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2  operator* (float s)       const { return {x * s,   y * s  }; }
    Vec2  operator/ (float s)       const { return {x / s,   y / s  }; }
    Vec2& operator+=(const Vec2& o)       { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o)       { x -= o.x; y -= o.y; return *this; }
    Vec2& operator*=(float s)             { x *= s;   y *= s;   return *this; }
    Vec2& operator/=(float s)             { x /= s;   y /= s;   return *this; }
    bool  operator==(const Vec2& o) const { return x == o.x && y == o.y; }
    bool  operator!=(const Vec2& o) const { return !(*this == o); }

    // ── Convenience ───────────────────────────────────────────────────────────

    // Integer screen-space x (truncates toward zero).
    int ix() const { return (int)x; }

    // Integer screen-space y (truncates toward zero).
    int iy() const { return (int)y; }

    // ── Vector math ──────────────────────────────────────────────────────────

    // Squared length (avoids sqrt — use for comparisons).
    //   if (vel.lengthSq() > MAX_SPEED*MAX_SPEED) { /* too fast */ }
    float lengthSq() const { return x * x + y * y; }

    // Actual length (uses sqrt — prefer lengthSq for comparisons).
    float length() const { return sqrtf(x * x + y * y); }

    // Manhattan distance (|x| + |y|). Good for grid-based games.
    float manhattan() const { return fabsf(x) + fabsf(y); }

    // Dot product.
    float dot(const Vec2& o) const { return x * o.x + y * o.y; }

    // Returns a unit vector. If length is zero, returns {0,0}.
    Vec2 normalized() const {
        float len = length();
        return len > 0.0001f ? Vec2{x / len, y / len} : Vec2{0, 0};
    }

    // Returns a vector with each component's sign: -1, 0, or +1.
    Vec2 sign() const {
        return {(float)((x > 0) - (x < 0)), (float)((y > 0) - (y < 0))};
    }

    // Component-wise clamp.
    Vec2 clamped(Vec2 lo, Vec2 hi) const {
        return {gclamp(x, lo.x, hi.x), gclamp(y, lo.y, hi.y)};
    }

    // Squared distance between two points.
    //   if (Vec2::distSq(a, b) < 64) { /* within 8 px */ }
    static float distSq(Vec2 a, Vec2 b) { return (a - b).lengthSq(); }

    // Distance between two points.
    static float dist(Vec2 a, Vec2 b) { return (a - b).length(); }
};


// ─── Rect ────────────────────────────────────────────────────────────────────
// Integer axis-aligned bounding box.
// Build one per object each frame; check with overlaps(). No state to keep.
//
// Example:
//   Rect dino {dinoX + 4, (int)dinoY + 2, 8, 12};
//   Rect obs  {(int)o.x,  topY,            w, h };
//   if (dino.overlaps(obs)) { /* hit! */ }
struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;

    // True when the two rects share at least one pixel.
    // Touching edges (x+w == o.x) do NOT count as overlap.
    bool overlaps(const Rect& o) const {
        return !(x + w <= o.x || o.x + o.w <= x ||
                 y + h <= o.y || o.y + o.h <= y);
    }

    // True if point (px, py) lies within the rect.
    bool contains(int px, int py) const {
        return px >= x && px < x + w && py >= y && py < y + h;
    }

    // Center point of the rect.
    Vec2 center() const {
        return {x + w * 0.5f, y + h * 0.5f};
    }

    // Returns a new Rect shrunk by dx pixels on the left AND right sides,
    // and by dy pixels on the top AND bottom. Use for hitbox insets.
    //
    //   Rect sprite {ox, oy, 16, 8};
    //   Rect hit = sprite.inset(2, 1);  // 2 px narrower each side, 1 px shorter each end
    Rect inset(int dx, int dy) const {
        return {x + dx, y + dy, w - 2 * dx, h - 2 * dy};
    }
};


// ─── Math helpers ─────────────────────────────────────────────────────────────

// Integer linear interpolation. Returns a + (b – a) * t / tmax.
// Useful for UI slide-ins, camera tracking, and score animations.
// t is clamped to [0, tmax] automatically.
//
//   int x = lerpi(0, 128, frame, 10);  // slide across screen over 10 frames
inline int lerpi(int a, int b, int t, int tmax) {
    t = gclamp(t, 0, tmax);
    return a + (b - a) * t / tmax;
}

// Float linear interpolation. Returns a + (b – a) * t.
// t is clamped to [0, 1] automatically.
//
//   float alpha = lerpf(0.0f, 1.0f, progress);
inline float lerpf(float a, float b, float t) {
    t = gclamp(t, 0.0f, 1.0f);
    return a + (b - a) * t;
}

// Map a value from one range to another.
//   float pct = mapRange(health, 0, maxHealth, 0.0f, 1.0f);
inline float mapRange(float v, float inMin, float inMax, float outMin, float outMax) {
    if (inMax == inMin) return outMin;
    return outMin + (v - inMin) * (outMax - outMin) / (inMax - inMin);
}

// Sign of a value: returns -1, 0, or +1.
// Useful for directional movement without a branch.
//
//   vel.x += gsign(target.x - pos.x) * ACCEL;
template<typename T>
inline int gsign(T v) {
    return (v > T(0)) - (v < T(0));
}

// Wrap an integer index into [0, count).
// Handles both positive overflow and negative underflow, so it works for
// both LEFT and RIGHT navigation in a cyclic menu.
//
//   selected = wrapIdx(selected - 1, gameCount);  // wrap left
//   selected = wrapIdx(selected + 1, gameCount);  // wrap right
inline uint8_t wrapIdx(int idx, uint8_t count) {
    if (count == 0) return 0;
    idx %= (int)count;
    if (idx < 0) idx += (int)count;
    return (uint8_t)idx;
}