#include "SystemMenu.h"

SystemMenu::SystemMenu(GameBase** games, const uint8_t* gameCount, Battery* batt)
    : _games(games), _gameCount(gameCount), _batt(batt) {}

void SystemMenu::onEnter(Console& ctx) {
    _running = true;
    _dirty = true;
    _launchedGame = nullptr;
    _lastInputTime = millis();
    _battPct = _batt->readPercent();
    _battTimer = millis();
}

void SystemMenu::onExit(Console& ctx) {}

bool        SystemMenu::isRunning()   const { return _running; }
const char* SystemMenu::getName()     const { return "SystemMenu"; }
bool        SystemMenu::needsRedraw() const { return _dirty; }
GameBase*   SystemMenu::getLaunchedGame() const { return _launchedGame; }

void SystemMenu::update(Console& ctx) {
    if (*_gameCount == 0) return;

    _dirty = true; // Force redraw every frame for animations

    // ── Check Input & Auto-Sleep ──
    bool anyInput = false;
    for (uint8_t i = 0; i < (uint8_t)Btn::COUNT; i++) {
        if (ctx.pressed((Btn)i)) { anyInput = true; break; }
    }
    
    if (anyInput) _lastInputTime = millis();
    else if (millis() - _lastInputTime >= IDLE_SLEEP_MS) _enterDeepSleep(ctx);

    // ── Battery Polling ──
    if (millis() - _battTimer >= 15000UL) {
        uint8_t newPct = _batt->readPercent();
        if (newPct != _battPct) {
            _battPct = newPct;
        }
        _battTimer = millis();
    }

    // ── Navigation (UPDATED FOR SLIDING) ──
    if (ctx.repeat(Btn::LEFT)) {
        _prevSelected = _selected;
        _selected = (_selected == 0) ? *_gameCount - 1 : _selected - 1;
        _slideOffset = -Console::W; // Start off-screen left
        ctx.sfxMenuNav();
    }
    if (ctx.repeat(Btn::RIGHT)) {
        _prevSelected = _selected;
        _selected = (_selected + 1) % *_gameCount;
        _slideOffset = Console::W;  // Start off-screen right
        ctx.sfxMenuNav();
    }

    // ── Animation Physics ──
    if (_slideOffset != 0) {
        int step = abs(_slideOffset) / 3; // Move 33% of remaining distance
        if (step < 4) step = 4;           // Minimum speed to snap to 0 cleanly
        
        if (_slideOffset > 0) {
            _slideOffset -= step;
            if (_slideOffset < 0) _slideOffset = 0;
        } else {
            _slideOffset += step;
            if (_slideOffset > 0) _slideOffset = 0;
        }
    }

    // ── Mute Toggle ──
    if (ctx.justPressed(Btn::MENU2)) {
        ctx.toggleMute();
        if (!ctx.isMuted()) ctx.beep(800, 40); 
    }

    // ── Launch Game ──
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuEnter();
        _launchedGame = _games[_selected];
        _running = false; 
    }
}

void SystemMenu::draw(Console& ctx) {
    if (*_gameCount == 0) {
        ctx.setFont(u8g2_font_6x10_tf);
        ctx.drawStr(4, 30, "No games loaded!");
        return;
    }
    
    _drawHeader(ctx);

    // If sliding, draw the outgoing card
    if (_slideOffset != 0) {
        // Calculate the opposite offset for the old card
        int prevOffset = (_slideOffset > 0) ? _slideOffset - Console::W : _slideOffset + Console::W;
        _drawGameCard(ctx, _prevSelected, prevOffset);
    }
    
    // Draw the incoming/current card
    _drawGameCard(ctx, _selected, _slideOffset);

    // Draw the static UI elements on top
    _drawPagination(ctx, _selected);
    _drawFooter(ctx);
    
    _dirty = false; 
}

void SystemMenu::_drawHeader(Console& ctx) {
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawStr(0, Layout::HDR_TEXT_Y, FW_NAME);

    // Mute Indicator
    if (ctx.isMuted()) {
        uint8_t nameW = ctx.strWidth(FW_NAME);
        ctx.drawStr(nameW + 3, Layout::HDR_TEXT_Y, "[M]");
    }

    // Battery Outline
    ctx.drawFrame(Layout::BATT_BOX_X, Layout::BATT_BOX_Y, Layout::BATT_BOX_W, Layout::BATT_BOX_H);
    ctx.drawBox(Layout::BATT_BOX_X + Layout::BATT_BOX_W, Layout::BATT_BOX_Y + 2, 2, 3);

    // Battery Fill Level
    uint8_t fill = (uint8_t)((uint16_t)_battPct * (Layout::BATT_BOX_W - 2) / 100);
    if (fill > 0) ctx.drawBox(Layout::BATT_BOX_X + 1, Layout::BATT_BOX_Y + 1, fill, Layout::BATT_BOX_H - 2);
    if (_batt->isCharging()) ctx.drawStr(Layout::BATT_BOX_X + 10, Layout::BATT_BOX_Y + 6, "+");

    // Battery Text
    char pctBuf[5];
    snprintf(pctBuf, sizeof(pctBuf), "%3u%%", _battPct);
    ctx.drawStr(Layout::BATT_TXT_X, Layout::HDR_TEXT_Y, pctBuf);
    
    // Bottom Border
    ctx.drawHLine(0, Layout::HDR_LINE_Y, Console::W);
}

void SystemMenu::_drawGameCard(Console& ctx, uint8_t idx, int offsetX) {
    const GameBase* g = _games[idx];
    const uint8_t* cover = g->getCoverArt();
    const uint8_t* icon = g->getIcon();

    // 1. Sleek Card Frame (Slides)
    ctx.drawRFrame(Layout::CARD_FRAME_X + offsetX, Layout::CARD_FRAME_Y, Layout::CARD_FRAME_W, Layout::CARD_FRAME_H, 4);

    if (cover) {
        // 2. Full-Card Cover Art (Slides)
        // Draw the 76x32 cover art with a 2px padding from the top-left of the card frame
        int coverX = Layout::CARD_FRAME_X + 2 + offsetX;
        int coverY = Layout::CARD_FRAME_Y + 2; 
        ctx.drawBitmap(coverX, coverY, Layout::CARD_COVER_BPR, Layout::CARD_COVER_H, cover);
    } else {
        // 2. Fallback Icon (Slides)
        int bounce = (millis() / 200) % 4;
        int floatOffset = (bounce == 3) ? 1 : bounce; 
        int iconY = Layout::CARD_ICON_Y - floatOffset;

        if (icon) {
            ctx.drawBitmap(Layout::CARD_ICON_X + offsetX, iconY, 2, Layout::CARD_ICON_SIZE, icon);
        } else {
            ctx.drawRFrame(Layout::CARD_ICON_X + offsetX, iconY, Layout::CARD_ICON_SIZE, Layout::CARD_ICON_SIZE, 3);
            char ini[2] = { g->getName()[0], '\0' };
            ctx.setFont(u8g2_font_7x13B_tf);
            ctx.drawStr(Layout::CARD_ICON_X + 4 + offsetX, iconY + 12, ini);
        }

        // 3. Fallback Title Text (Slides)
        // Only drawn if there is no cover art
        ctx.setFont(u8g2_font_7x13B_tf);
        const char* name = g->getName();
        uint8_t nameW = ctx.strWidth(name);
        ctx.drawStr((Console::W - nameW) / 2 + offsetX, Layout::CARD_NAME_Y, name);
    }
}

void SystemMenu::_drawPagination(Console& ctx, uint8_t idx) {
    // 4. Pagination Dots (Static)
    int dotSpacing = 8;
    int totalW = (*_gameCount - 1) * dotSpacing;
    int startX = (Console::W - totalW) / 2;
    
    for (uint8_t i = 0; i < *_gameCount; i++) {
        if (i == idx) {
            ctx.drawDisc(startX + (i * dotSpacing), Layout::CARD_PAGE_Y - 2, 2); 
        } else {
            ctx.drawCircle(startX + (i * dotSpacing), Layout::CARD_PAGE_Y - 2, 2); 
        }
    }

    // 5. Breathing Navigation Arrows (Static)
    ctx.setFont(u8g2_font_6x10_tf);
    int arrowPulse = (millis() / 250) % 2; 
    
    if (idx > 0) {
        ctx.drawStr(Layout::ARROW_L_X - arrowPulse, Layout::ARROW_Y, "<");
    }
    if (idx < *_gameCount - 1) {
        ctx.drawStr(Layout::ARROW_R_X + arrowPulse, Layout::ARROW_Y, ">");
    }
}

void SystemMenu::_drawFooter(Console& ctx) {
    ctx.drawHLine(0, Layout::FTR_LINE_Y, Console::W);
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawStr(Layout::FTR_TEXT_X, Layout::FTR_TEXT_Y, "[A]Launch  [<>]Browse");
}

void SystemMenu::_enterDeepSleep(Console& ctx) {
    ctx.setDrawColor(0);
    ctx.drawBox(0, 0, Console::W, Console::H);
    ctx.setDrawColor(1);
    ctx.setFont(u8g2_font_6x10_tf);
    uint8_t w = ctx.strWidth("Sleeping...");
    ctx.drawStr((Console::W - w) / 2, 34, "Sleeping...");
    
    // We use the escape hatch here to force the buffer to send immediately before power cut
    ctx.gfx().sendBuffer(); 
    delay(1000);

    ctx.stopSound();
    ctx.gfx().setPowerSave(1); 
    esp_deep_sleep_start();
}