#pragma once
#include "Console.h"

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
            ctx.drawBitmapEx(Console::W / 2 - iconW / 2, ty + 5, iconW, iconH, icon, 0, false, false, 1);
        }

        ctx.setFont(u8g2_font_4x6_tr);
        if ((millis() / 500) % 2 == 0) {
            ctx.drawStrCentered(55, "PRESS A TO START");
        }
    }

    // Standard Game Over screen with score
    inline void drawGameOver(Console& ctx, uint32_t score, uint32_t highScore, bool isNewHigh = false) {
        ctx.pushDrawState();
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(10, 10, Console::W - 20, Console::H - 20);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(10, 10, Console::W - 20, Console::H - 20);
        
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
