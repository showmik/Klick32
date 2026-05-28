#pragma once
#include "Console.h"
#include "GameUtils.h"

// ─── UI ──────────────────────────────────────────────────────────────────────
// Reusable user interface widgets (menus, bars, dialogs).
// ─────────────────────────────────────────────────────────────────────────────
namespace UI {

    // Draw a health/progress bar. progress is 0.0 to 1.0.
    inline void drawBar(Console& ctx, int x, int y, int w, int h, float progress) {
        ctx.drawFrame(x, y, w, h);
        int fillW = (int)((w - 2) * progress);
        if (fillW > 0) {
            ctx.drawBox(x + 1, y + 1, fillW, h - 2);
        }
    }

    // Draw a score with a label
    inline void drawScore(Console& ctx, int x, int y, uint32_t score, const char* label = "SCORE") {
        ctx.setFont(u8g2_font_4x6_tr);
        ctx.drawStr(x, y, label);
        ctx.setFont(u8g2_font_6x10_tf);
        ctx.drawPrintf(x, y + 9, "%lu", (unsigned long)score);
    }

    // A simple vertical menu. Returns true if an item was selected this frame.
    // Call every frame. 'selected' is updated on D-pad input.
    inline bool updateAndDrawMenu(Console& ctx, const char** items, uint8_t count, uint8_t& selected, int yStart) {
        if (ctx.justPressed(Btn::UP)) {
            selected = (selected == 0) ? count - 1 : selected - 1;
            ctx.sfxMenuNav();
        } else if (ctx.justPressed(Btn::DOWN)) {
            selected = (selected + 1) % count;
            ctx.sfxMenuNav();
        }

        ctx.setFont(u8g2_font_6x10_tf);
        int lh = 12; // line height
        
        for (uint8_t i = 0; i < count; i++) {
            int y = yStart + i * lh;
            if (i == selected) {
                int w = ctx.strWidth(items[i]);
                int cx = Console::W / 2;
                ctx.drawBox(cx - w / 2 - 4, y - 9, w + 8, 11);
                ctx.setDrawColor(Console::COLOR_BLACK);
                ctx.drawStrCentered(y, items[i]);
                ctx.setDrawColor(Console::COLOR_WHITE);
            } else {
                ctx.drawStrCentered(y, items[i]);
            }
        }

        if (ctx.justPressed(Btn::A)) {
            ctx.sfxMenuEnter();
            return true;
        }
        return false;
    }

}
