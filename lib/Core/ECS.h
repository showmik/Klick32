#pragma once
#include <stdint.h>

// ─── Lightweight ECS (Entity Component System) ──────────────────────────────
// A zero-allocation, Structure-of-Arrays (SoA) ECS designed for microcontrollers.
// Replaces inheritance trees (e.g., class Bullet : public Entity) with pure data.
// ─────────────────────────────────────────────────────────────────────────────

typedef uint16_t EntityID;
constexpr EntityID INVALID_ENTITY = 0xFFFF;

// The Registry manages the lifecycle of Entity IDs.
template<uint16_t MAX_ENTITIES>
class ECSRegistry {
public:
    bool alive[MAX_ENTITIES] = {false};

    ECSRegistry() = default;

    // Creates a new entity and returns its ID. Returns INVALID_ENTITY if full.
    EntityID create() {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            if (!alive[i]) {
                alive[i] = true;
                return i;
            }
        }
        return INVALID_ENTITY;
    }

    // Marks an entity as dead. 
    // Note: Component pools don't strictly need to be cleared; their data will 
    // simply be ignored or overwritten the next time this ID is recycled.
    void destroy(EntityID e) {
        if (e < MAX_ENTITIES) {
            alive[e] = false;
        }
    }

    void clear() {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            alive[i] = false;
        }
    }

    bool isValid(EntityID e) const {
        return e < MAX_ENTITIES && alive[e];
    }
};

// A ComponentPool stores contiguous data for a specific component type.
template<typename T, uint16_t MAX_ENTITIES>
class ComponentPool {
public:
    T data[MAX_ENTITIES];
    bool has[MAX_ENTITIES] = {false};

    ComponentPool() = default;

    // Attaches a component to an entity.
    void add(EntityID e, const T& component) {
        if (e < MAX_ENTITIES) {
            data[e] = component;
            has[e] = true;
        }
    }

    // Removes a component from an entity.
    void remove(EntityID e) {
        if (e < MAX_ENTITIES) {
            has[e] = false;
        }
    }

    // Clears all components (useful on game reset)
    void clear() {
        for (uint16_t i = 0; i < MAX_ENTITIES; i++) {
            has[i] = false;
        }
    }

    // Retrieves a pointer to the component, or nullptr if it doesn't exist.
    T* get(EntityID e) {
        if (e < MAX_ENTITIES && has[e]) return &data[e];
        return nullptr;
    }

    const T* get(EntityID e) const {
        if (e < MAX_ENTITIES && has[e]) return &data[e];
        return nullptr;
    }
};

// ─── Example Usage ────────────────────────────────────────────────────────────
// struct Transform { float x, y; };
// struct Physics   { float vx, vy; };
//
// ECSRegistry<64> registry;
// ComponentPool<Transform, 64> transforms;
// ComponentPool<Physics, 64> physics;
//
// EntityID player = registry.create();
// transforms.add(player, {10.0f, 20.0f});
// physics.add(player, {1.5f, 0.0f});
//
// // System iteration loop:
// for (EntityID e = 0; e < 64; e++) {
//     if (registry.isValid(e) && transforms.has[e] && physics.has[e]) {
//         transforms.data[e].x += physics.data[e].vx * dt;
//         transforms.data[e].y += physics.data[e].vy * dt;
//     }
// }
// ─────────────────────────────────────────────────────────────────────────────
