#include "DinoGame.h"
#include <U8g2lib.h>

// ─── Sprites ──────────────────────────────────────────────────────────────────
// All bitmaps: MSB-first, big-endian rows.
// For 16-wide sprites: 2 bytes per row.
// Bit mapping: col 0 = MSB of byte 0, col 7 = LSB of byte 0,
//              col 8 = MSB of byte 1, col 15 = LSB of byte 1.
//
// Dino faces RIGHT. Head is upper-right of the sprite.

// ── Running dino (16×16) ──────────────────────────────────────────────────────
static const uint8_t PROGMEM spr_run1[32] = {
    0x01, 0xF0,   // .......XXXXX....  head top
    0x01, 0xFC,   // .......XXXXXXX..  head
    0x01, 0xBC,   // .......XX.XXXX..  head (eye gap)
    0x0F, 0xFC,   // ....XXXXXXXXXX..  neck
    0x1F, 0xFC,   // ...XXXXXXXXXXX..  upper body
    0x1F, 0xF0,   // ...XXXXXXXXX....  body
    0x0F, 0xE0,   // ....XXXXXXX.....  body
    0x07, 0xC0,   // .....XXXXX......  body
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,
    0x07, 0xC0,   // .....XXXXX......  body base
    0x05, 0x00,   // .....X.X........  legs split
    0x05, 0x00,
    0x0D, 0x80,   // ....XX.XX.......  feet
    0x00, 0x00,
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
    0x06, 0x00,   // .....XX.........  legs together
    0x06, 0x00,
    0x0F, 0x00,   // ....XXXX........  feet together
    0x00, 0x00,
};

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

// ── Ducking dino (16×8) ───────────────────────────────────────────────────────
// Crouched body — head + compressed torso + legs.
// 2 bytes per row, 8 rows = 16 bytes per frame.

static const uint8_t PROGMEM spr_duck1[16] = {
    0x01, 0xF0,   // .......XXXXX....  head
    0x01, 0xBC,   // .......XX.XXXX..  head + eye
    0x0F, 0xFC,   // ....XXXXXXXXXX..  neck
    0x1F, 0xE0,   // ...XXXXXXXXX....  upper body
    0x0F, 0xC0,   // ....XXXXXXX.....  lower body
    0x05, 0x00,   // .....X.X........  legs split
    0x0D, 0x80,   // ....XX.XX.......  feet
    0x00, 0x00,
};

static const uint8_t PROGMEM spr_duck2[16] = {
    0x01, 0xF0,
    0x01, 0xBC,
    0x0F, 0xFC,
    0x1F, 0xE0,
    0x0F, 0xC0,
    0x06, 0x00,   // .....XX.........  legs together
    0x0F, 0x00,   // ....XXXX........  feet together
    0x00, 0x00,
};

// ── Pterodactyl (16×8) ────────────────────────────────────────────────────────
// Symmetric bird shape. 2 bytes per row, 8 rows = 16 bytes per frame.
//
// Frame 0 — wings up:
//   col: 0123456789ABCDEF
//   ..X.......X.....   tips up
//   ..XX.....XX.....   wings rising
//   ..XXXXXXXXX.....   full span
//   .....XXXXX......   body
//   .......X........   beak / tail
//
// Frame 1 — wings down (mirrored vertically):
//   .......X........   beak
//   .....XXXXX......   body
//   ..XXXXXXXXX.....   full span
//   ..XX.....XX.....   wings dropping
//   ..X.......X.....   tips down

static const uint8_t PROGMEM spr_ptero1[16] = {
    0x20, 0x20,   // ..X.......X.....
    0x30, 0x30,   // ..XX.....XX.....
    0x3F, 0xE0,   // ..XXXXXXXXX.....
    0x07, 0xC0,   // .....XXXXX......
    0x01, 0x00,   // .......X........
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
};

static const uint8_t PROGMEM spr_ptero2[16] = {
    0x01, 0x00,   // .......X........
    0x07, 0xC0,   // .....XXXXX......
    0x3F, 0xE0,   // ..XXXXXXXXX.....
    0x30, 0x30,   // ..XX.....XX.....
    0x20, 0x20,   // ..X.......X.....
    0x00, 0x00,
    0x00, 0x00,
    0x00, 0x00,
};

// ── Small cactus (8×16) ───────────────────────────────────────────────────────
static const uint8_t PROGMEM spr_cactus_s[16] = {
    0x30, 0x30, 0x30, 0x30,
    0xFC, 0xFC,   // left arm
    0x3F, 0x3F,   // right arm
    0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30,
};

// ── Large cactus (12×16) ──────────────────────────────────────────────────────
static const uint8_t PROGMEM spr_cactus_l[32] = {
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x7F, 0xC0,   // left arm
    0x7F, 0xC0,
    0x0F, 0xF0,   // right arm
    0x0F, 0xF0,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
    0x0C, 0x00,
};

// ─── AABB collision helper ────────────────────────────────────────────────────
static bool aabbOverlap(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh) {
    return !(ax + aw <= bx || bx + bw <= ax ||
             ay + ah <= by || by + bh <= ay);
}

// ─── DinoGame implementation ──────────────────────────────────────────────────

void DinoGame::onEnter() {
    _initRound();
    _running = true;
}

void DinoGame::onExit() {}

// ─── _initRound ──────────────────────────────────────────────────────────────
void DinoGame::_initRound() {
    _dinoY          = (float)(GROUND_Y - DINO_H);
    _dinoVY         = 0.0f;
    _onGround       = true;
    _isDucking      = false;
    _coyoteFrames   = 0;
    _jumpBuffer     = 0;
    _score          = 0;
    _lastMilestone  = 0;
    _flashTimer     = 0;
    _speed          = INIT_SPEED;
    _frameCnt       = 0;
    _animTimer      = 0;
    _animFrame      = 0;
    _state          = DinoState::RUNNING;

    for (auto& o : _obs)   o.active = false;
    for (auto& p : _ptero) p.active = false;

    // Space clouds evenly across the screen at varied heights
    _clouds[0] = { 15.0f,  14 };
    _clouds[1] = { 62.0f,  19 };
    _clouds[2] = { 105.0f, 13 };

    _spawnObsIfNeeded();
}

// ─── _spawnObsIfNeeded ────────────────────────────────────────────────────────
void DinoGame::_spawnObsIfNeeded() {
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
        o.x     = base + (float)random(MIN_GAP, MAX_GAP + 1);
        o.large = (random(4) == 0);   // 25 % large
        o.active = true;
        break;
    }
}

// ─── _spawnPteroIfNeeded ──────────────────────────────────────────────────────
void DinoGame::_spawnPteroIfNeeded() {
    if (_score < PTERO_MIN_SCORE) return;

    for (auto& p : _ptero)
        if (p.active) return;   // one at a time

    // ~1 % chance per frame  →  appears roughly every 3 seconds
    if (random(100) != 0) return;

    _ptero[0].x          = (float)(SCREEN_W + 16);
    _ptero[0].heightIdx  = (uint8_t)(random(2));   // 0 = low, 1 = high
    _ptero[0].animFrame  = 0;
    _ptero[0].animTimer  = 0;
    _ptero[0].active     = true;
}

// ─── _checkObsCollision ───────────────────────────────────────────────────────
bool DinoGame::_checkObsCollision(const Obstacle& o) const {
    // Dino hitbox — same whether standing or ducking on x-axis
    int dx, dy, dw, dh;
    if (_isDucking) {
        dx = DINO_X + 3;
        dy = GROUND_Y - DUCK_H + 1;
        dw = DINO_W - 5;
        dh = DUCK_H - 2;
    } else {
        dx = DINO_X + 4;
        dy = (int)_dinoY + 2;
        dw = DINO_W - 8;
        dh = DINO_H - 4;
    }

    int cw = o.large ? LARGE_W : SMALL_W;
    int cx = (int)o.x + 1;
    int cy = GROUND_Y - CACTUS_H;
    int ch = CACTUS_H - 1;

    return aabbOverlap(dx, dy, dw, dh, cx, cy, cw, ch);
}

// ─── _checkPteroCollision ─────────────────────────────────────────────────────
bool DinoGame::_checkPteroCollision(const Pterodactyl& p) const {
    // Dino hitbox
    int dx, dy, dw, dh;
    if (_isDucking) {
        dx = DINO_X + 3;
        dy = GROUND_Y - DUCK_H + 1;
        dw = DINO_W - 5;
        dh = DUCK_H - 2;
    } else {
        dx = DINO_X + 4;
        dy = (int)_dinoY + 2;
        dw = DINO_W - 8;
        dh = DINO_H - 4;
    }

    int py = (p.heightIdx == 0) ? PTERO_LOW_Y : PTERO_HIGH_Y;
    return aabbOverlap(dx, dy, dw, dh,
                       (int)p.x + 2, py + 1,
                       PTERO_W - 4,  PTERO_H - 2);
}

// ─── _drawCloud ───────────────────────────────────────────────────────────────
// Three overlapping filled circles — classic side-scroller cloud silhouette.
void DinoGame::_drawCloud(U8G2& disp, int x, int y) const {
    disp.drawDisc(x + 4,  y + 5, 3);
    disp.drawDisc(x + 9,  y + 3, 4);
    disp.drawDisc(x + 15, y + 5, 3);
}

// ─── update ──────────────────────────────────────────────────────────────────
void DinoGame::update(InputManager& input, Sound& sound) {

    bool jumpPressed = input.justPressed(Btn::UP)  || input.justPressed(Btn::A);
    bool wantDuck    = input.held(Btn::DOWN)        || input.held(Btn::B);
    bool menuPressed = input.justPressed(Btn::MENU1);

    switch (_state) {

        // ── RUNNING ──────────────────────────────────────────────────────────
        case DinoState::RUNNING: {

            // Duck — only valid on ground; suppresses jump input
            _isDucking = wantDuck && _onGround;

            // ── Jump buffer ───────────────────────────────────────────────────
            // Remember a jump press for JUMP_BUFFER_FRAMES even if in the air.
            if (jumpPressed) _jumpBuffer = JUMP_BUFFER_FRAMES;
            if (_jumpBuffer > 0) _jumpBuffer--;

            // ── Coyote time ───────────────────────────────────────────────────
            // Count down from COYOTE_FRAMES after leaving the ground.
            if (_onGround) {
                _coyoteFrames = COYOTE_FRAMES;   // keep refreshing while grounded
            } else {
                if (_coyoteFrames > 0) _coyoteFrames--;
            }

            // ── Can we jump? ──────────────────────────────────────────────────
            // Conditions: a buffered press exists AND
            //             either on the ground or within the coyote window AND
            //             not ducking.
            bool canJump = (_jumpBuffer > 0) && (_coyoteFrames > 0) && !_isDucking;

            if (canJump) {
                _dinoVY       = JUMP_VY;
                _onGround     = false;
                _coyoteFrames = 0;   // consume the coyote window
                _jumpBuffer   = 0;   // consume the buffered press
                SFX::jump(sound);
            }

            // Physics
            _dinoVY += GRAVITY;
            _dinoY  += _dinoVY;
            const float groundPos = (float)(GROUND_Y - DINO_H);
            if (_dinoY >= groundPos) {
                _dinoY    = groundPos;
                _dinoVY   = 0.0f;
                _onGround = true;
            }

            // Speed ramp
            if (_speed < MAX_SPEED) _speed += SPEED_INC;

            // ── Obstacles ────────────────────────────────────────────────────
            for (auto& o : _obs) {
                if (!o.active) continue;
                o.x -= _speed;
                if (o.x + (o.large ? LARGE_W : SMALL_W) < 0)
                    o.active = false;
            }
            _spawnObsIfNeeded();

            // ── Pterodactyls ─────────────────────────────────────────────────
            for (auto& p : _ptero) {
                if (!p.active) continue;
                p.x -= _speed;
                if (p.x + PTERO_W < 0) { p.active = false; continue; }
                // Animate wings every 8 frames
                if (++p.animTimer >= 8) {
                    p.animTimer = 0;
                    p.animFrame ^= 1;
                }
            }
            _spawnPteroIfNeeded();

            // ── Clouds ───────────────────────────────────────────────────────
            for (auto& c : _clouds) {
                c.x -= _speed * 0.25f;   // slow parallax
                if (c.x < -22.0f) {
                    c.x = (float)(SCREEN_W + random(10, 40));
                    c.y = (int8_t)(12 + random(10));
                }
            }

            // ── Score ────────────────────────────────────────────────────────
            _score++;
            if (_score > _hiScore) _hiScore = _score;

            // Milestone flash + beep every SCORE_MILESTONE points
            if (_score % SCORE_MILESTONE == 0 && _score != _lastMilestone) {
                _lastMilestone = _score;
                _flashTimer    = FLASH_FRAMES;
                SFX::point(sound);
            }
            if (_flashTimer > 0) _flashTimer--;

            // ── Running animation (toggle every 8 frames) ────────────────────
            if (++_animTimer >= 8) {
                _animTimer = 0;
                _animFrame ^= 1;
            }
            _frameCnt++;

            // ── Collision detection ───────────────────────────────────────────
            for (auto& o : _obs) {
                if (o.active && _checkObsCollision(o)) {
                    _state = DinoState::DEAD;
                    SFX::death(sound);
                    break;
                }
            }
            if (_state == DinoState::RUNNING) {
                for (auto& p : _ptero) {
                    if (p.active && _checkPteroCollision(p)) {
                        _state = DinoState::DEAD;
                        SFX::death(sound);
                        break;
                    }
                }
            }
            break;
        }

        // ── DEAD ─────────────────────────────────────────────────────────────
        case DinoState::DEAD:
            _isDucking = false;
            if (jumpPressed) _initRound();
            break;
    }

    if (menuPressed) _running = false;
}

// ─── draw ────────────────────────────────────────────────────────────────────
void DinoGame::draw(U8G2& disp) {

    // ── Clouds (draw first — background layer) ────────────────────────────────
    for (auto& c : _clouds)
        _drawCloud(disp, (int)c.x, (int)c.y);

    // ── Ground line ───────────────────────────────────────────────────────────
    disp.drawHLine(0, GROUND_Y, SCREEN_W);

    // ── Scrolling ground texture ──────────────────────────────────────────────
    int offset = (int)((float)_frameCnt * _speed) % 20;
    for (int x = -offset; x < SCREEN_W; x += 20) {
        disp.drawHLine(x + 3,  GROUND_Y + 2, 6);
        disp.drawHLine(x + 13, GROUND_Y + 4, 3);
    }

    // ── Dino sprite ───────────────────────────────────────────────────────────
    if (_state == DinoState::DEAD) {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, spr_dead);

    } else if (_isDucking) {
        // Duck: snap sprite to ground regardless of physics Y
        int duckY = GROUND_Y - DUCK_H;
        const uint8_t* duckSpr = (_animFrame == 0) ? spr_duck1 : spr_duck2;
        disp.drawBitmap(DINO_X, duckY, 2, DUCK_H, duckSpr);

    } else if (!_onGround) {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, spr_run1);

    } else {
        const uint8_t* runSpr = (_animFrame == 0) ? spr_run1 : spr_run2;
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, runSpr);
    }

    // ── Obstacles ─────────────────────────────────────────────────────────────
    for (auto& o : _obs) {
        if (!o.active) continue;
        int ox = (int)o.x;
        int oy = GROUND_Y - CACTUS_H;
        if (o.large)
            disp.drawBitmap(ox, oy, 2, CACTUS_H, spr_cactus_l);
        else
            disp.drawBitmap(ox, oy, 1, CACTUS_H, spr_cactus_s);
    }

    // ── Pterodactyls ──────────────────────────────────────────────────────────
    for (auto& p : _ptero) {
        if (!p.active) continue;
        int py = (p.heightIdx == 0) ? PTERO_LOW_Y : PTERO_HIGH_Y;
        const uint8_t* spr = (p.animFrame == 0) ? spr_ptero1 : spr_ptero2;
        disp.drawBitmap((int)p.x, py, 2, PTERO_H, spr);
    }

    // ── HUD: score ────────────────────────────────────────────────────────────
    char buf[12];
    disp.setFont(u8g2_font_6x10_tf);

    if (_flashTimer > 0) {
        // Flash: invert the score block
        disp.setDrawColor(1);
        disp.drawBox(64, 0, 64, 22);
        disp.setDrawColor(0);
        snprintf(buf, sizeof(buf), "HI:%05u", (unsigned)_hiScore);
        disp.drawStr(68, 9, buf);
        snprintf(buf, sizeof(buf), "%05u", (unsigned)_score);
        disp.drawStr(86, 20, buf);
        disp.setDrawColor(1);
    } else {
        snprintf(buf, sizeof(buf), "HI:%05u", (unsigned)_hiScore);
        disp.drawStr(68, 9, buf);
        snprintf(buf, sizeof(buf), "%05u", (unsigned)_score);
        disp.drawStr(86, 20, buf);
    }

    // ── Overlay: GAME OVER ────────────────────────────────────────────────────
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

// ─── GameBase interface ───────────────────────────────────────────────────────

bool DinoGame::isRunning() const { return _running; }

const char* DinoGame::getName() const { return "Dino Run"; }

const uint8_t* DinoGame::getIcon() const { return nullptr; }