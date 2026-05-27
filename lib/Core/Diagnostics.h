#pragma once
#include <Arduino.h>
#include "Console.h"
#include "GameBase.h"

// ─── Diagnostics ──────────────────────────────────────────────────────────────
// A toggleable HUD overlay that renders performance metrics.
// Shows FPS, Free Heap, Max Allocated Block, and CPU logic time.
// ─────────────────────────────────────────────────────────────────────────────

class Diagnostics {
public:
    static void begin() {
        _lastTime = millis();
        _frameCount = 0;
        _fps = 0;
    }

    // Call at the very beginning of the game loop
    static void markUpdateStart() {
        _updateStart = micros();
    }

    // Call after game->update() but before drawing
    static void markUpdateEnd() {
        _updateEnd = micros();
    }

    // Call at the very end of the loop, right before yield/delay
    static void tick() {
        _frameCount++;
        uint32_t now = millis();
        if (now - _lastTime >= 1000) {
            _fps = _frameCount;
            _frameCount = 0;
            _lastTime = now;
        }
    }

    // Renders the diagnostics overlay on top of everything
    static void draw(Console& ctx) {
        if (!_visible) return;

        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(0, 0, 128, 16);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(0, 0, 128, 16);

        ctx.setFont(u8g2_font_4x6_tr); // Tiny font
        
        char buf[64];
        
        // FPS and Logic Time
        uint32_t logicTimeUs = _updateEnd - _updateStart;
        snprintf(buf, sizeof(buf), "FPS: %d  Logic: %lu us", _fps, logicTimeUs);
        ctx.drawStr(2, 6, buf);

        // Memory usage
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maxBlock = ESP.getMaxAllocHeap();
        snprintf(buf, sizeof(buf), "RAM: %lu B / Max: %lu B", freeHeap, maxBlock);
        ctx.drawStr(2, 13, buf);
    }

    static void toggle() {
        _visible = !_visible;
    }

    static bool isVisible() {
        return _visible;
    }

private:
    static bool _visible;
    static uint32_t _lastTime;
    static uint32_t _frameCount;
    static uint16_t _fps;
    
    static uint32_t _updateStart;
    static uint32_t _updateEnd;
};
