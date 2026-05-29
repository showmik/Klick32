#include "SystemMenu.h"

SystemMenu::SystemMenu(GameRecord* games, const uint8_t* gameCount, Battery* batt)
    : _games(games), _gameCount(gameCount), _batt(batt) {}

void SystemMenu::onEnter(Console& ctx) {
    _running = true;
    _dirty = true;
    _launchedGame = nullptr;
    _battPct = _batt->readPercent();
    _battTimer = millis();
}

void SystemMenu::onExit(Console& ctx) {}

bool        SystemMenu::isRunning()   const { return _running; }
const char* SystemMenu::getName()     const { return "SystemMenu"; }
bool        SystemMenu::needsRedraw() const { return _dirty; }
GameRecord* SystemMenu::getLaunchedGameRecord() const { return _launchedGame; }

void SystemMenu::update(Console& ctx, float dt) {
    if (*_gameCount == 0) return;

    // ── Animation Tick ──
    uint8_t newAnimTick = (millis() / 200) % 4;
    uint8_t newArrowTick = (millis() / 250) % 2;
    if (newAnimTick != _animTick) { _animTick = newAnimTick; _dirty = true; }
    if (newArrowTick != _arrowTick) { _arrowTick = newArrowTick; _dirty = true; }

    // ── Input Reset for Dirty Flag ──
    for (uint8_t i = 0; i < (uint8_t)Btn::COUNT; i++) {
        if (ctx.pressed((Btn)i)) { _dirty = true; break; }
    }

    // ── Battery Polling ──
    if (millis() - _battTimer >= 15000UL) {
        uint8_t newPct = _batt->readPercent();
        if (newPct != _battPct) {
            _battPct = newPct;
            _dirty = true;
        }
        _battTimer = millis();
    }
    
    // ── About Page Logic ──
    if (_inAboutPage) {
        if (ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU1)) {
            _inAboutPage = false;
            ctx.sfxMenuBack();
            _dirty = true;
        }
        return; // Skip normal menu logic
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
        _dirty = true;
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
    if (ctx.justPressed(Btn::MENU2) && !ctx.pressed(Btn::MENU1)) {
        ctx.toggleMute();
        if (!ctx.isMuted()) ctx.beep(800, 40); 
        ctx.saveBool("mute", ctx.isMuted()); // Persist setting
    }
    
    // ── Open About Page ──
    if (ctx.justPressed(Btn::B)) {
        _inAboutPage = true;
        ctx.sfxMenuEnter();
        _dirty = true;
    }

    // ── Launch Game ──
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        _launchedGame = &_games[_selected];
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
    
    if (_inAboutPage) {
        _drawAboutPage(ctx);
        _dirty = false;
        return;
    }

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
    const GameRecord& g = _games[idx];
    const uint8_t* cover = g.cover;
    const uint8_t* icon = g.icon;

    // 1. Sharp Card Frame (Slides)
    ctx.drawFrame(Layout::CARD_FRAME_X + offsetX, Layout::CARD_FRAME_Y, Layout::CARD_FRAME_W, Layout::CARD_FRAME_H);

    if (cover) {
        // 2. Full-Card Cover Art (Slides) - Padding removed!
        int coverX = Layout::CARD_FRAME_X + offsetX;
        int coverY = Layout::CARD_FRAME_Y; 
        ctx.drawBitmap(coverX, coverY, Layout::CARD_COVER_BPR, Layout::CARD_COVER_H, cover);
    } else {
        // ... Keep existing fallback Icon & Text logic here ...
        int bounce = (millis() / 200) % 4;
        int floatOffset = (bounce == 3) ? 1 : bounce; 
        int iconY = Layout::CARD_ICON_Y - floatOffset;

        if (icon) {
            ctx.drawBitmap(Layout::CARD_ICON_X + offsetX, iconY, 2, Layout::CARD_ICON_SIZE, icon);
        } else {
            ctx.drawRFrame(Layout::CARD_ICON_X + offsetX, iconY, Layout::CARD_ICON_SIZE, Layout::CARD_ICON_SIZE, 3);
            char ini[2] = { g.name[0], '\0' };
            ctx.setFont(u8g2_font_7x13B_tf);
            ctx.drawStr(Layout::CARD_ICON_X + 4 + offsetX, iconY + 12, ini);
        }

        ctx.setFont(u8g2_font_7x13B_tf);
        const char* name = g.name;
        uint8_t nameW = ctx.strWidth(name);
        ctx.drawStr((Console::W - nameW) / 2 + offsetX, Layout::CARD_NAME_Y, name);
    }
}

void SystemMenu::_drawPagination(Console& ctx, uint8_t idx) {
    // 4. Breathing Navigation Arrows (Static Overlay)
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
    
    // Left Text
    ctx.drawStr(Layout::FTR_TEXT_X, Layout::FTR_TEXT_Y, "[B]About [A]Play");

    // Right Text (Game Count)
    char countBuf[10];
    snprintf(countBuf, sizeof(countBuf), "%u/%u", _selected + 1, *_gameCount);
    int w = ctx.strWidth(countBuf);
    ctx.drawStr(Console::W - w - Layout::FTR_TEXT_X, Layout::FTR_TEXT_Y, countBuf);
}

void SystemMenu::_drawAboutPage(Console& ctx) {
    // 1. Draw a premium solid white header bar with black text
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.drawBox(0, 0, Console::W, 11);
    
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawStrCentered(8, "SYSTEM INFORMATION");
    
    // 2. Draw refined tabular system info in white
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.setFont(u8g2_font_4x6_tr); // Sleek tiny font
    
    // Left column (labels) at x = 6
    ctx.drawStr(6, 22, "OS Name:");
    ctx.drawStr(6, 30, "Firmware:");
    ctx.drawStr(6, 38, "Processor:");
    ctx.drawStr(6, 46, "System RAM:");
    
    // Right column (values) starting at x = 58
    ctx.drawStr(58, 22, FW_NAME " OS");
    ctx.drawStr(58, 30, FW_VERSION);
    
#ifdef SIMULATOR
    ctx.drawStr(58, 38, "PC (Simulator)");
    ctx.drawStr(58, 46, "N/A (Virtual)");
#else
    ctx.drawStr(58, 38, "ESP32-S3");
    
    uint32_t freeHeap = ESP.getFreeHeap() / 1024;
    char ramStr[24];
    snprintf(ramStr, sizeof(ramStr), "%lu KB Free", freeHeap);
    ctx.drawStr(58, 46, ramStr);
#endif

    // 3. Draw a tiny battery indicator widget aligned in the top right
    char battStr[12];
    if (_batt->isCharging()) {
        snprintf(battStr, sizeof(battStr), "%u%% +", _battPct);
    } else {
        snprintf(battStr, sizeof(battStr), "%u%%", _battPct);
    }
    int strW = ctx.strWidth(battStr);
    ctx.drawStr(108 - strW, 22, battStr);
    
    ctx.drawFrame(112, 17, 10, 6); // outline
    ctx.drawBox(122, 19, 1, 2);    // tip
    
    int fillW = (int)(8 * (_battPct / 100.0f));
    if (fillW > 8) fillW = 8;
    ctx.drawBox(113, 18, fillW, 4);

    // 4. Footer override for About Page
    ctx.drawHLine(0, Layout::FTR_LINE_Y, Console::W);
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawStr(Layout::FTR_TEXT_X, Layout::FTR_TEXT_Y, "[B/M1] Back");
}


