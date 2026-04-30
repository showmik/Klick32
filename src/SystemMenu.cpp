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
    if (*_gameCount == 0) return; // Wait for games to register

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
            _dirty = true;
        }
        _battTimer = millis();
    }

    // ── Navigation ──
    if (ctx.repeat(Btn::LEFT)) {
        _selected = (_selected == 0) ? *_gameCount - 1 : _selected - 1;
        _dirty = true;
        ctx.sfxMenuNav();
    }
    if (ctx.repeat(Btn::RIGHT)) {
        _selected = (_selected + 1) % *_gameCount;
        _dirty = true;
        ctx.sfxMenuNav();
    }

    // ── Mute Toggle ──
    if (ctx.justPressed(Btn::MENU2)) {
        ctx.toggleMute();
        _dirty = true;
        if (!ctx.isMuted()) ctx.beep(800, 40); // unmute sfx
    }

    // ── Launch Game ──
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuEnter();
        _launchedGame = _games[_selected];
        _running = false; // Exits the menu, triggering the OS to launch the game
    }
}

void SystemMenu::draw(Console& ctx) {
    if (*_gameCount == 0) {
        ctx.setFont(u8g2_font_6x10_tf);
        ctx.drawStr(4, 30, "No games loaded!");
        return;
    }
    _drawHeader(ctx);
    _drawGameCard(ctx, _selected);
    _drawFooter(ctx);
    _dirty = false; // Reset dirty flag after drawing
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

void SystemMenu::_drawGameCard(Console& ctx, uint8_t idx) {
    const GameBase* g = _games[idx];
    const uint8_t* icon = g->getIcon();

    // Icon rendering
    if (icon) {
        ctx.drawBitmap(Layout::CARD_ICON_X, Layout::CARD_ICON_Y, 2, Layout::CARD_ICON_SIZE, icon);
    } else {
        ctx.drawRFrame(Layout::CARD_ICON_X, Layout::CARD_ICON_Y, Layout::CARD_ICON_SIZE, Layout::CARD_ICON_SIZE, 3);
        char ini[2] = { g->getName()[0], '\0' };
        ctx.setFont(u8g2_font_7x13B_tf);
        ctx.drawStr(Layout::CARD_ICON_X + 4, Layout::CARD_ICON_Y + 12, ini);
    }

    // Title
    ctx.setFont(u8g2_font_7x13B_tf);
    const char* name = g->getName();
    uint8_t nameW = ctx.strWidth(name);
    ctx.drawStr((Console::W - nameW) / 2, Layout::CARD_NAME_Y, name);

    // Pagination
    ctx.setFont(u8g2_font_5x7_tf);
    char pg[8];
    snprintf(pg, sizeof(pg), "%u / %u", (unsigned)(idx + 1), (unsigned)*_gameCount);
    uint8_t pgW = ctx.strWidth(pg);
    ctx.drawStr((Console::W - pgW) / 2, Layout::CARD_PAGE_Y, pg);

    // Navigation Arrows
    ctx.setFont(u8g2_font_6x10_tf);
    if (idx > 0)                ctx.drawStr(Layout::ARROW_L_X, Layout::ARROW_Y, "<");
    if (idx < *_gameCount - 1)  ctx.drawStr(Layout::ARROW_R_X, Layout::ARROW_Y, ">");
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