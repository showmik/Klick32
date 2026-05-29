#pragma once
#include "Console.h"
#include <stdint.h>

struct Particle {
    float x = 0;
    float y = 0;
    float vx = 0;
    float vy = 0;
    uint8_t life = 0;
    uint8_t startLife = 0;
    bool active = false;
};

// ─── ParticleManager ─────────────────────────────────────────────────────────
// Manages transient visual effects like particles, sparks, and blood spurts.
// Removes the need for games to manually track particle arrays and lifespans.
// ─────────────────────────────────────────────────────────────────────────────
class ParticleManager {
public:
    static constexpr int MAX_PARTICLES = 32;

    ParticleManager() = default;

    // Spawn a basic pixel particle with velocity and lifespan
    void spawnPixel(float x, float y, float vx, float vy, uint8_t life);

    // Update particle positions and lifespans (call once per frame).
    // dt is the frame delta in seconds (currently unused but accepted for
    // future delta-time-based particle physics).
    void update(float dt = 0.0f);

    // Draw all active particles
    void draw(Console& ctx) const;

    // Instantly remove all particles
    void clear();

private:
    Particle _particles[MAX_PARTICLES];
};
