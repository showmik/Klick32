#pragma once
#include "Console.h"
#include "ParticleManager.h"

// ─── CommonScreens ───────────────────────────────────────────────────────────
// Reusable screen layouts to prevent boilerplate copy-pasting across games.
// ─────────────────────────────────────────────────────────────────────────────
namespace Screens {

    // Standard "Press A to Start" title screen
    inline void drawTitle(Console& ctx, const char* title, const uint8_t* icon = nullptr, int iconW = 0, int iconH = 0) {
        ctx.setFont(u8g2_font_8x13_tf);
        int ty = (icon) ? 20 : 30;
        ctx.drawStrCentered(ty, title);

        if (icon) {
            ctx.blit(icon, (iconW + 7) / 8, iconH).at(Console::W / 2 - (iconW / 2), ty + 5).draw();
        }

        ctx.setFont(u8g2_font_4x6_tr);
        if ((millis() / 500) % 2 == 0) {
            ctx.drawStrCentered(55, "PRESS A TO START");
        }
    }

    // Standard Game Over screen with score
    inline void drawGameOver(Console& ctx, uint32_t score, uint32_t highScore, bool isNewHigh = false, ParticleManager* particles = nullptr) {
        ctx.pushDrawState();
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(6, 6, Console::W - 12, Console::H - 12);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(6, 6, Console::W - 12, Console::H - 12);
        
        ctx.setFont(u8g2_font_6x10_tf);
        ctx.drawStrCentered(22, "GAME OVER");
        
        ctx.setFont(u8g2_font_4x6_tr);
        ctx.drawPrintfCentered(34, "SCORE: %lu", (unsigned long)score);
        
        if (isNewHigh && ((millis() / 300) % 2 == 0)) {
            ctx.drawStrCentered(44, "NEW HIGH SCORE!");
        } else {
            ctx.drawPrintfCentered(44, "BEST: %lu", (unsigned long)highScore);
        }
        
        if ((millis() / 500) % 2 == 0) {
            ctx.drawStrCentered(56, "PRESS A TO CONTINUE");
        }

        // Draw dynamic high score fanfare particles
        if (isNewHigh && particles) {
            // Spawn 1 pixel spark every few frames to create a steady celebration fanfare
            if (random(0, 100) < 35) {
                float px = 10 + random(0, Console::W - 20);
                float py = 52;
                float vx = ((float)random(0, 100) / 100.0f) * 1.2f - 0.6f;
                float vy = -((float)random(0, 100) / 100.0f) * 0.9f - 0.4f; // shoot upwards
                uint8_t life = random(10, 20);
                particles->spawnPixel(px, py, vx, vy, life);
            }
            particles->draw(ctx);
        }

        ctx.popDrawState();
    }

    // Standard semi-transparent Pause overlay
    inline void drawPauseOverlay(Console& ctx) {
        ctx.pushDrawState();
        ctx.drawDitherBox(0, 0, Console::W, Console::H, 2); // 50% dither
        
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(30, 24, 68, 16);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(30, 24, 68, 16);
        
        ctx.setFont(u8g2_font_6x10_tf);
        ctx.drawStrCentered(35, "PAUSED");
        ctx.popDrawState();
    }

}
