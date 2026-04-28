#include "DinoGame.h"
#include <U8g2lib.h>

// ─── Sprites ─────────────────────────────────────────────────────────────────
// All bitmaps: MSB-first, big-endian rows.
// For 16-wide sprites: 2 bytes per row, 16 rows = 32 bytes total.
// Bit mapping: col 0 = MSB of byte 0, col 7 = LSB of byte 0,
//              col 8 = MSB of byte 1, col 15 = LSB of byte 1.
//
// Dino faces RIGHT. Head is upper-right of the sprite.
//
// Legend (each comment shows the 16-pixel row):
//   col:  0123456789ABCDEF

static const uint8_t PROGMEM spr_run1[32] = {
    0x01, 0xF0,   // .......XXXXX....  head top
    0x01, 0xFC,   // .......XXXXXXX..  head
    0x01, 0xBC,   // .......XX.XXXX..  head (eye gap at col 9)
    0x0F, 0xFC,   // ....XXXXXXXXXX..  neck
    0x1F, 0xFC,   // ...XXXXXXXXXXX..  upper body
    0x1F, 0xF0,   // ...XXXXXXXXX....  body
    0x0F, 0xE0,   // ....XXXXXXX.....  body
    0x07, 0xC0,   // .....XXXXX......  body
    0x07, 0xC0,   // .....XXXXX......
    0x07, 0xC0,   // .....XXXXX......
    0x07, 0xC0,   // .....XXXXX......
    0x07, 0xC0,   // .....XXXXX......  body base
    0x05, 0x00,   // .....X.X........  legs (split)
    0x05, 0x00,   // .....X.X........
    0x0D, 0x80,   // ....XX.XX.......  feet
    0x00, 0x00,   // ................
};

static const uint8_t PROGMEM spr_run2[32] = {
    0x01, 0xF0,
    0x01, 0xFC,
    0x01, 0xBC,
    0x0F, 0xFC,
    0x1F, 0xFC,
    0x1F, 0xF0,
    0x0F, 0xE0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x06, 0x00,   // .....XX.........  legs (together)
    0x06, 0x00,   // .....XX.........
    0x0F, 0x00,   // ....XXXX........  feet (together)
    0x00, 0x00,
};

// Dead dino: eye row unchanged → fill it (closed eye)
static const uint8_t PROGMEM spr_dead[32] = {
    0x01, 0xF0,
    0x01, 0xFC,
    0x01, 0xFC,   // .......XXXXXXX..  eye closed
    0x0F, 0xFC,
    0x1F, 0xFC,
    0x1F, 0xF0,
    0x0F, 0xE0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0x00,   // .....XXX........  legs tucked
    0x07, 0x00,
    0x07, 0x00,
    0x00, 0x00,
};

// Small cactus: 8 px wide, 1 byte per row, 16 rows
// Stem = cols 2-3 (0x30 = ..XX....)
static const uint8_t PROGMEM spr_cactus_s[16] = {
    0x30,   // ..XX....  upper stem
    0x30,
    0x30,
    0x30,
    0xFC,   // XXXXXX..  left arm
    0xFC,
    0x3F,   // ..XXXXXX  right arm
    0x3F,
    0x30,   // ..XX....  lower stem
    0x30,
    0x30,
    0x30,
    0x30,
    0x30,
    0x30,
    0x30,
};

// Large cactus: 12 px wide, 2 bytes per row, 16 rows = 32 bytes
// Stem = cols 4-5 (0x0C, 0x00 = ....XX......)
static const uint8_t PROGMEM spr_cactus_l[32] = {
    0x0C, 0x00,   // ....XX......  upper stem
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x7F, 0xC0,   // .XXXXXXXXX..  left arm (cols 1-9)
    0x7F, 0xC0,
    0x0F, 0xF0,   // ....XXXXXXXX  right arm (cols 4-11)
    0x0F, 0xF0,
    0x0C, 0x00,   // ....XX......  lower stem
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
};

// ─── Helpers ─────────────────────────────────────────────────────────────────

static bool aabbOverlap(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh) {
    return !(ax + aw <= bx || bx + bw <= ax ||
             ay + ah <= by || by + bh <= ay);
}

// ─── DinoGame Implementation ───────────────────────────────────────────────

void DinoGame::onEnter() {
    _initRound();
    _running = true;
}

void DinoGame::onExit() {
    // Nothing to clean up
}

void DinoGame::_initRound() {
    _dinoY     = (float)(GROUND_Y - DINO_H);
    _dinoVY    = 0.0f;
    _onGround  = true;
    _score     = 0;
    _speed     = INIT_SPEED;
    _frameCnt  = 0;
    _animTimer = 0;
    _animFrame = 0;
    _state     = DinoState::RUNNING;
    for (auto& o : _obs) o.active = false;
    _spawnIfNeeded();
}

void DinoGame::_spawnIfNeeded() {
    // Find rightmost edge of all active obstacles
    float rightmost = -1.0f;
    uint8_t nActive = 0;
    for (auto& o : _obs) {
        if (!o.active) continue;
        nActive++;
        float edge = o.x + (o.large ? LARGE_W : SMALL_W);
        if (edge > rightmost) rightmost = edge;
    }

    bool shouldSpawn = (nActive == 0) ||
                       (nActive < MAX_OBS && rightmost < (float)(SCREEN_W + MAX_GAP));

    if (!shouldSpawn) return;

    for (auto& o : _obs) {
        if (o.active) continue;
        float base = (rightmost < (float)SCREEN_W) ? (float)SCREEN_W : rightmost;
        o.x      = base + (float)random(MIN_GAP, MAX_GAP + 1);
        o.large  = (random(4) == 0);   // 25 % chance of large cactus
        o.active = true;
        break;
    }
}

bool DinoGame::_checkCollision(const Obstacle& o) const {
    // Dino hitbox (inset from sprite to be forgiving)
    int dx = DINO_X + 4;
    int dy = (int)_dinoY + 2;
    int dw = DINO_W - 8;   // 8 px wide hit area
    int dh = DINO_H - 4;

    int cw  = o.large ? LARGE_W : SMALL_W;
    int cx  = (int)o.x + 1;
    int cy  = GROUND_Y - CACTUS_H;
    int ch  = CACTUS_H - 1;

    return aabbOverlap(dx, dy, dw, dh, cx, cy, cw, ch);
}

void DinoGame::update(InputManager& input, Sound& sound) {
    // ── Read inputs ──────────────────────────────────────────────────────────
    bool jumpPressed = input.justPressed(Btn::UP) || input.justPressed(Btn::A);
    bool menuPressed = input.justPressed(Btn::MENU1) || input.justPressed(Btn::B);

    // ── State Machine ────────────────────────────────────────────────────────
    switch (_state) {

        // ── Running ──────────────────────────────────────────────────────────
        case DinoState::RUNNING: {
            // Jump
            if (jumpPressed && _onGround) {
                _dinoVY  = JUMP_VY;
                _onGround = false;
            }

            // Physics
            _dinoVY += GRAVITY;
            _dinoY  += _dinoVY;
            float groundPos = (float)(GROUND_Y - DINO_H);
            if (_dinoY >= groundPos) {
                _dinoY    = groundPos;
                _dinoVY   = 0.0f;
                _onGround = true;
            }

            // Ramp up speed
            if (_speed < MAX_SPEED) _speed += SPEED_INC;

            // Move obstacles + recycle off-screen ones
            for (auto& o : _obs) {
                if (!o.active) continue;
                o.x -= _speed;
                if (o.x + (o.large ? LARGE_W : SMALL_W) < 0)
                    o.active = false;
            }

            // Spawn next obstacle when needed
            _spawnIfNeeded();

            // Score
            _score++;
            if (_score > _hiScore) _hiScore = _score;

            // Animate running legs (toggle every 8 frames)
            if (++_animTimer >= 8) {
                _animTimer = 0;
                _animFrame ^= 1;
            }

            _frameCnt++;

            // Collision detection
            for (auto& o : _obs) {
                if (o.active && _checkCollision(o)) {
                    _state = DinoState::DEAD;
                    break;
                }
            }
            break;
        }

        // ── Dead ─────────────────────────────────────────────────────────────
        case DinoState::DEAD:
            if (jumpPressed) {
                _initRound();
            }
            break;
    }

    // Exit to menu?
    if (menuPressed) {
        _running = false;
    }
}

void DinoGame::draw(U8G2& disp) {
    // ── Ground line ──────────────────────────────────────────────────────────
    disp.drawHLine(0, GROUND_Y, SCREEN_W);

    // ── Scrolling ground texture ─────────────────────────────────────────────
    int offset = (int)((float)_frameCnt * _speed) % 20;
    for (int x = -offset; x < SCREEN_W; x += 20) {
        disp.drawHLine(x + 3, GROUND_Y + 2, 6);
        disp.drawHLine(x + 13, GROUND_Y + 4, 3);
    }

    // ── Dino sprite ──────────────────────────────────────────────────────────
    int dy = (int)_dinoY;
    if (_state == DinoState::DEAD) {
        disp.drawBitmap(DINO_X, dy, 2, DINO_H, spr_dead);
    } else if (!_onGround) {
        disp.drawBitmap(DINO_X, dy, 2, DINO_H, spr_run1);
    } else {
        disp.drawBitmap(DINO_X, dy, 2, DINO_H,
                        (_animFrame == 0) ? spr_run1 : spr_run2);
    }

    // ── Obstacles ────────────────────────────────────────────────────────────
    for (auto& o : _obs) {
        if (!o.active) continue;
        int ox = (int)o.x;
        int oy = GROUND_Y - CACTUS_H;
        if (o.large)
            disp.drawBitmap(ox, oy, 2, CACTUS_H, spr_cactus_l);
        else
            disp.drawBitmap(ox, oy, 1, CACTUS_H, spr_cactus_s);
    }

    // ── HUD: score ───────────────────────────────────────────────────────────
    char buf[12];
    disp.setFont(u8g2_font_6x10_tf);
    snprintf(buf, sizeof(buf), "HI:%05u", (unsigned)_hiScore);
    disp.drawStr(68, 9, buf);
    snprintf(buf, sizeof(buf), "%05u", (unsigned)_score);
    disp.drawStr(86, 20, buf);

    // ── Overlay: game over ───────────────────────────────────────────────────
    if (_state == DinoState::DEAD) {
        disp.setDrawColor(0);
        disp.drawBox(18, 20, 92, 28);
        disp.setDrawColor(1);
        disp.drawFrame(18, 20, 92, 28);
        disp.setFont(u8g2_font_7x13B_tf);
        disp.drawStr(22, 36, "GAME  OVER");
        disp.setFont(u8g2_font_6x10_tf);
        disp.drawStr(26, 46, "A to restart");
    }
}

bool DinoGame::isRunning() const {
    return _running;
}

const char* DinoGame::getName() const {
    return "Dino Run";
}

const uint8_t* DinoGame::getIcon() const {
    return nullptr; // Use default placeholder
}