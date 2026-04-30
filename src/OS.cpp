#include "OS.h"

// ─── Constructor ─────────────────────────────────────────────────────────────
// _console is initialised last, after _disp / _input / _sound, matching the
// declaration order in OS.h.  The references it holds are valid from this
// point forward even though begin() has not been called yet.
OS::OS()
    : _disp(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA)
    , _console(_disp, _input, _sound)
{}

// ─── begin ───────────────────────────────────────────────────────────────────
void OS::begin() {
    Serial.begin(115200);
    Wire.begin(PIN_SDA, PIN_SCL);
    _disp.begin();
    _input.begin();
    _sound.begin();
    _batt.begin();
    randomSeed(esp_random());
    _battPct   = _batt.readPercent();
    _battTimer = millis();
}

// ─── registerGame ────────────────────────────────────────────────────────────
void OS::registerGame(GameBase* game) {
    if (_gameCount < MAX_GAMES && game != nullptr)
        _games[_gameCount++] = game;
}

// ─── run ─────────────────────────────────────────────────────────────────────
void OS::run() {
    if (_gameCount == 0) {
        _disp.clearBuffer();
        _disp.setFont(u8g2_font_6x10_tf);
        _disp.drawStr(4, 30, "No games loaded!");
        _disp.drawStr(4, 44, "Register a game");
        _disp.drawStr(4, 58, "in main.cpp.");
        _disp.sendBuffer();
        while (true) delay(1000);
    }

    GameBase* activeGame = nullptr;

    while (true) {
        uint32_t t0 = millis();
        _input.update();

        if (millis() - _battTimer >= 15000UL) {
            _battPct   = _batt.readPercent();
            _battTimer = millis();
        }

        // ════════════════════════════════════════════════════════════════════
        if (activeGame == nullptr) {
            // ── MENU ─────────────────────────────────────────────────────────

            if (_input.repeat(Btn::LEFT)) {
                _selected = (_selected == 0) ? _gameCount - 1 : _selected - 1;
                SFX::menuNav(_sound);
            }
            if (_input.repeat(Btn::RIGHT)) {
                _selected = (_selected + 1) % _gameCount;
                SFX::menuNav(_sound);
            }

            if (_input.justPressed(Btn::MENU2)) {
                _sound.toggleMute();
                if (!_sound.isMuted()) SFX::unmute(_sound);
            }

            if (_input.justPressed(Btn::A) || _input.justPressed(Btn::MENU1)) {
                SFX::menuEnter(_sound);
                activeGame = _games[_selected];
                activeGame->onEnter();
                uint32_t elapsed = millis() - t0;
                if (elapsed < FRAME_MS) delay(FRAME_MS - elapsed);
                continue;
            }

            _drawMenu();

        } else {
            // ── IN GAME ──────────────────────────────────────────────────────

            activeGame->update(_console);

            _disp.clearBuffer();
            activeGame->draw(_console);
            _disp.sendBuffer();

            if (!activeGame->isRunning()) {
                activeGame->onExit();
                activeGame = nullptr;
                SFX::menuBack(_sound);
            }
        }
        // ════════════════════════════════════════════════════════════════════

        uint32_t elapsed = millis() - t0;
        if (elapsed < FRAME_MS) delay(FRAME_MS - elapsed);
    }
}

// ─── Menu rendering ──────────────────────────────────────────────────────────

void OS::_drawMenu() {
    _disp.clearBuffer();
    _drawHeader();
    _drawGameCard(_selected);
    _drawFooter();
    _disp.sendBuffer();
}

void OS::_drawHeader() {
    _disp.setFont(u8g2_font_5x7_tf);
    _disp.drawStr(0, 7, FW_NAME);

    if (_sound.isMuted()) {
        uint8_t nameW = _disp.getStrWidth(FW_NAME);
        _disp.drawStr(nameW + 3, 7, "[M]");
    }

    const uint8_t bx = 98, by = 1, bw = 28, bh = 7;
    _disp.drawFrame(bx, by, bw, bh);
    _disp.drawBox(bx + bw, by + 2, 2, 3);

    uint8_t fill = (uint8_t)((uint16_t)_battPct * (bw - 2) / 100);
    if (fill > 0) _disp.drawBox(bx + 1, by + 1, fill, bh - 2);

    if (_batt.isCharging()) _disp.drawStr(bx + 10, by + 6, "+");

    char pctBuf[5];
    snprintf(pctBuf, sizeof(pctBuf), "%3u%%", _battPct);
    _disp.drawStr(74, 7, pctBuf);

    _disp.drawHLine(0, 9, SCREEN_W);
}

void OS::_drawGameCard(uint8_t idx) {
    if (idx >= _gameCount) return;
    const GameBase* g = _games[idx];

    const uint8_t iconX = 56, iconY = 12;
    const uint8_t* icon = g->getIcon();

    if (icon) {
        _disp.drawBitmap(iconX, iconY, 2, 16, icon);
    } else {
        _disp.drawRFrame(iconX, iconY, 16, 16, 3);
        char ini[2] = { g->getName()[0], '\0' };
        _disp.setFont(u8g2_font_7x13B_tf);
        _disp.drawStr(iconX + 4, iconY + 12, ini);
    }

    _disp.setFont(u8g2_font_7x13B_tf);
    const char* name = g->getName();
    uint8_t nameW = _disp.getStrWidth(name);
    _disp.drawStr((SCREEN_W - nameW) / 2, 41, name);

    _disp.setFont(u8g2_font_5x7_tf);
    char pg[8];
    snprintf(pg, sizeof(pg), "%u / %u", (unsigned)(idx + 1), (unsigned)_gameCount);
    uint8_t pgW = _disp.getStrWidth(pg);
    _disp.drawStr((SCREEN_W - pgW) / 2, 50, pg);

    _disp.setFont(u8g2_font_6x10_tf);
    if (idx > 0)               _disp.drawStr(0,   32, "<");
    if (idx < _gameCount - 1)  _disp.drawStr(122, 32, ">");
}

void OS::_drawFooter() {
    _disp.drawHLine(0, 53, SCREEN_W);
    _disp.setFont(u8g2_font_5x7_tf);
    _disp.drawStr(4, 61, "[A]Launch  [<>]Browse");
}