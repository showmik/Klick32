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

        ctx.beginScreenSpace();
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(0, 0, 128, 9);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawHLine(0, 8, 128);

        // Draw debug shapes
        ctx.endScreenSpace(); // shapes are usually world-space
        ctx.pushDrawState();
        for (int i = 0; i < _rectCount; i++) {
            ctx.setDrawColor(_rects[i].color);
            ctx.drawFrame(_rects[i].r.x, _rects[i].r.y, _rects[i].r.w, _rects[i].r.h);
        }
        _rectCount = 0;
        ctx.popDrawState();
        ctx.beginScreenSpace();

        ctx.setFont(u8g2_font_4x6_tr); // Tiny font
        
        uint32_t logicTimeUs = _updateEnd - _updateStart;

        // Render compact single-line performance diagnostics
#ifndef SIMULATOR
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t maxBlock = ESP.getMaxAllocHeap();
        uint32_t freeHeapKb = freeHeap / 1024;
        uint32_t maxBlockKb = maxBlock / 1024;
        
        ctx.drawPrintf(2, 6, "%uF | %lu.%lums | R:%luK | M:%luK", 
                       _fps, 
                       logicTimeUs / 1000, (logicTimeUs % 1000) / 100, 
                       freeHeapKb, 
                       maxBlockKb);
#else
        ctx.drawPrintf(2, 6, "%uF | %lu.%lums | PC Sim", 
                       _fps, 
                       logicTimeUs / 1000, (logicTimeUs % 1000) / 100);
#endif
        ctx.endScreenSpace();
    }

    static void toggle() {
        _visible = !_visible;
    }

    // Draw a debug rectangle for one frame
    static void drawRect(Rect r, uint8_t color = Console::COLOR_XOR) {
        if (!_visible || _rectCount >= MAX_DEBUG_RECTS) return;
        _rects[_rectCount].r = r;
        _rects[_rectCount].color = color;
        _rectCount++;
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

    struct DebugRect { Rect r; uint8_t color; };
    static constexpr int MAX_DEBUG_RECTS = 16;
    static DebugRect _rects[MAX_DEBUG_RECTS];
    static int _rectCount;
};

// ─── Profiling Macro ────────────────────────────────────────────────────────
// Use PROFILE_SCOPE("Name") at the top of a function or block to measure
// its execution time and print it to the Serial monitor.
// ─────────────────────────────────────────────────────────────────────────────
class ProfileScope {
    const char* _name;
    uint32_t _start;
public:
    ProfileScope(const char* name) : _name(name), _start(micros()) {}
    ~ProfileScope() {
        uint32_t diff = micros() - _start;
        Serial.printf("[Profile] %s: %lu us\n", _name, diff);
    }
};
#define PROFILE_SCOPE(name) ProfileScope _prof_##__LINE__(name)
