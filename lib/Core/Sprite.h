#pragma once
#include "Console.h"
#include <stdint.h>

// ─── Sprite ──────────────────────────────────────────────────────────────────
// A core engine entity representing a drawable object with position, bounds,
// and collision detection.
// ─────────────────────────────────────────────────────────────────────────────
class Sprite {
public:
    float x = 0.0f;
    float y = 0.0f;
    int width = 0;
    int height = 0;
    const uint8_t* bitmap = nullptr;
    int bytesPerRow = 1;
    bool active = true;

    Sprite() = default;
    Sprite(float startX, float startY, int w, int h, const uint8_t* bmp, int bpr = 1);

    // Draw the sprite using the provided Console context
    void draw(Console& ctx) const;

    // AABB (Axis-Aligned Bounding Box) collision detection against another sprite
    bool collidesWith(const Sprite& other) const;

    // Point collision detection
    bool contains(float px, float py) const;
};
