#pragma once
#include <stdint.h>
#include "Console.h"

// ─── Entity & EntityManager ───────────────────────────────────────────────────
// A lightweight, zero-allocation entity framework.
// Uses a pre-allocated pool to strictly avoid heap fragmentation (new/delete).
// ─────────────────────────────────────────────────────────────────────────────

class Entity {
public:
    bool active = false;

    virtual ~Entity() = default;
    
    // Called when the entity is spawned from the pool
    virtual void init() {}
    
    virtual void update(Console& ctx, float dt) {}
    virtual void draw(Console& ctx) {}
    
    void destroy() { active = false; }
};

template<typename T, uint16_t MAX_ENTITIES>
class EntityManager {
public:
    EntityManager() {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            _pool[i].active = false;
        }
    }

    // Spawns a new entity from the pool and calls init()
    // Returns nullptr if the pool is full.
    T* spawn() {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            if (!_pool[i].active) {
                _pool[i].active = true;
                _pool[i].init();
                return &_pool[i];
            }
        }
        return nullptr; // Pool exhausted
    }

    void update(Console& ctx, float dt) {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            if (_pool[i].active) {
                _pool[i].update(ctx, dt);
            }
        }
    }

    void draw(Console& ctx) {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            if (_pool[i].active) {
                _pool[i].draw(ctx);
            }
        }
    }

    void clear() {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            _pool[i].active = false;
        }
    }
    
    uint16_t count() const {
        uint16_t c = 0;
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            if (_pool[i].active) c++;
        }
        return c;
    }

    // Direct access for custom spatial queries or collision loops
    T* get(uint16_t index) {
        return &_pool[index];
    }
    
    constexpr uint16_t capacity() const {
        return MAX_ENTITIES;
    }

private:
    T _pool[MAX_ENTITIES];
};
