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

// ─── Collision Helpers ────────────────────────────────────────────────────────
// Eliminates O(N*M) nested loop boilerplate in games.
// Usage: 
//   checkCollisions(bullets, enemies, [](Bullet& b, Enemy& e) {
//       if (b.rect().overlaps(e.rect())) {
//           b.destroy(); e.destroy();
//       }
//   });
// ─────────────────────────────────────────────────────────────────────────────
template<typename T, uint16_t N, typename U, uint16_t M, typename Func>
inline void checkCollisions(EntityManager<T, N>& poolA, EntityManager<U, M>& poolB, Func onHit) {
    for (uint16_t i = 0; i < N; i++) {
        T* a = poolA.get(i);
        if (!a->active) continue;
        for (uint16_t j = 0; j < M; j++) {
            U* b = poolB.get(j);
            if (!b->active) continue;
            onHit(*a, *b);
            if (!a->active) break; // If 'a' was destroyed, skip remaining 'b's
        }
    }
}
