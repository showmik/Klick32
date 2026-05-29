#pragma once
#include "GameUtils.h"

// ─── PhysicsEngine ────────────────────────────────────────────────────────────
// Robust physics calculations for the Klick32 engine.
// Provides Swept-AABB collision detection to prevent high-speed tunneling
// and handles sub-pixel precision movement.
// ─────────────────────────────────────────────────────────────────────────────

struct RaycastResult {
    bool hit;
    float time;      // [0.0, 1.0] where the collision occurred
    float normalX;   // Surface normal X (-1, 0, 1)
    float normalY;   // Surface normal Y (-1, 0, 1)
};

class PhysicsEngine {
public:
    // Swept AABB collision between a moving box and a static box.
    // Returns a RaycastResult indicating if/when the collision occurs within this frame.
    // Use this to prevent tunneling for fast-moving objects (like bullets or falling players).
    static RaycastResult sweptAABB(const Rect& movingBox, const Vec2& velocity, const Rect& staticBox) {
        RaycastResult res = {false, 1.0f, 0.0f, 0.0f};

        float invEntryX, invEntryY;
        float invExitX, invExitY;

        // Find the distance between the objects on the near and far sides for both X and Y
        if (velocity.x > 0.0f) {
            invEntryX = staticBox.x - (movingBox.x + movingBox.w);
            invExitX  = (staticBox.x + staticBox.w) - movingBox.x;
        } else {
            invEntryX = (staticBox.x + staticBox.w) - movingBox.x;
            invExitX  = staticBox.x - (movingBox.x + movingBox.w);
        }

        if (velocity.y > 0.0f) {
            invEntryY = staticBox.y - (movingBox.y + movingBox.h);
            invExitY  = (staticBox.y + staticBox.h) - movingBox.y;
        } else {
            invEntryY = (staticBox.y + staticBox.h) - movingBox.y;
            invExitY  = staticBox.y - (movingBox.y + movingBox.h);
        }

        // Find time of collision and time of leaving for each axis
        float entryTimeX, entryTimeY;
        float exitTimeX, exitTimeY;

        if (velocity.x == 0.0f) {
            if (movingBox.x + movingBox.w <= staticBox.x || staticBox.x + staticBox.w <= movingBox.x) {
                entryTimeX = -std::numeric_limits<float>::infinity();
                exitTimeX  = -std::numeric_limits<float>::infinity();
            } else {
                entryTimeX = -std::numeric_limits<float>::infinity();
                exitTimeX  = std::numeric_limits<float>::infinity();
            }
        } else {
            entryTimeX = invEntryX / velocity.x;
            exitTimeX  = invExitX / velocity.x;
        }

        if (velocity.y == 0.0f) {
            if (movingBox.y + movingBox.h <= staticBox.y || staticBox.y + staticBox.h <= movingBox.y) {
                entryTimeY = -std::numeric_limits<float>::infinity();
                exitTimeY  = -std::numeric_limits<float>::infinity();
            } else {
                entryTimeY = -std::numeric_limits<float>::infinity();
                exitTimeY  = std::numeric_limits<float>::infinity();
            }
        } else {
            entryTimeY = invEntryY / velocity.y;
            exitTimeY  = invExitY / velocity.y;
        }

        // Find the earliest/latest times of collision
        float entryTime = max(entryTimeX, entryTimeY);
        float exitTime  = min(exitTimeX, exitTimeY);

        // Check if there was no collision
        if (entryTime > exitTime || (entryTimeX < 0.0f && entryTimeY < 0.0f) || entryTimeX > 1.0f || entryTimeY > 1.0f) {
            return res; // No hit
        } else {
            // Collision occurred
            res.hit = true;
            res.time = entryTime;
            
            // Calculate normals
            if (entryTimeX > entryTimeY) {
                if (invEntryX < 0.0f) { res.normalX = 1.0f; res.normalY = 0.0f; }
                else                  { res.normalX = -1.0f; res.normalY = 0.0f; }
            } else {
                if (invEntryY < 0.0f) { res.normalX = 0.0f; res.normalY = 1.0f; }
                else                  { res.normalX = 0.0f; res.normalY = -1.0f; }
            }
            return res;
        }
    }
};
