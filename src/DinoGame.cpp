#include "DinoGame.h"
#include <U8g2lib.h>

// ─── Sprites ──────────────────────────────────────────────────────────────────
// All bitmaps: MSB-first, big-endian rows.
// For 16-wide sprites: 2 bytes per row.
//   col 0 = MSB of byte 0, col 7 = LSB of byte 0
//   col 8 = MSB of byte 1, col 15 = LSB of byte 1
//
// Dino faces RIGHT; head is upper-right of the sprite.

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
    0x07, 0xC0,   //                   body base
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
    0x01, 0xFC,   // eye closed
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
// Symmetric bird shape. 2 bytes/row, 8 rows = 16 bytes per frame.
//
// Frame 0 — wings up:
//   ..X.......X.....   tips up
//   ..XX.....XX.....   wings rising
//   ..XXXXXXXXX.....   full span
//   .....XXXXX......   body
//   .......X........   body centre / beak hint
//
// Frame 1 — wings down (rows flipped):
//   .......X........
//   .....XXXXX......
//   ..XXXXXXXXX.....
//   ..XX.....XX.....
//   ..X.......X.....
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

// ─── AABB helper ─────────────────────────────────────────────────────────────
static bool aabbOverlap(int ax, int ay, int aw, int ah,
                        int bx, int by, int bw, int bh) {
    return !(ax + aw <= bx || bx + bw <= ax ||
             ay + ah <= by || by + bh <= ay);
}

// ─── Per-kind static queries ─────────────────────────────────────────────────

/*static*/ int DinoGame::_obsWidth(ObstacleKind k) {
    switch (k) {
        case ObstacleKind::CACTUS_LARGE: return LARGE_W;
        case ObstacleKind::PTERO_LOW:
        case ObstacleKind::PTERO_HIGH:   return PTERO_W;
        default:                          return SMALL_W;
    }
}

// Top pixel row of the obstacle when rendered.
/*static*/ int DinoGame::_obsTopY(ObstacleKind k) {
    switch (k) {
        case ObstacleKind::PTERO_LOW:  return PTERO_LOW_Y;
        case ObstacleKind::PTERO_HIGH: return PTERO_HIGH_Y;
        default:                        return GROUND_Y - CACTUS_H;  // 36
    }
}

/*static*/ bool DinoGame::_isPtero(ObstacleKind k) {
    return k == ObstacleKind::PTERO_LOW || k == ObstacleKind::PTERO_HIGH;
}

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void DinoGame::onEnter() { _initRound(); _running = true; }
void DinoGame::onExit()  {}

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

    for (auto& o : _obs) o.active = false;

    _clouds[0] = {  15.0f, 14 };
    _clouds[1] = {  62.0f, 19 };
    _clouds[2] = { 105.0f, 13 };

    _spawnObsIfNeeded();
}

// ─── _spawnObsIfNeeded ────────────────────────────────────────────────────────
//
// This is the only spawn function.  It picks the obstacle type at spawn time,
// so cacti and pterodactyls always respect the same gap budget and can never
// overlap or appear in impossible combinations.
//
// Rules:
//   1. Spawn when the pipeline is empty, or when there is room for a second
//      queued obstacle (right edge of current rightmost < SCREEN_W + MAX_GAP).
//   2. Pterodactyls are unlocked after PTERO_MIN_SCORE.
//   3. Pterodactyls never appear back-to-back — the rightmost active obstacle's
//      kind is checked before rolling the type.
//   4. Among eligible types: 25 % pterodactyl / 75 % cactus (once unlocked).
//      Of cactus: 25 % large, 75 % small.
void DinoGame::_spawnObsIfNeeded() {
    // Scan active obstacles to find the rightmost right-edge and its kind.
    float        rightmost  = -1.0f;
    uint8_t      nActive    = 0;
    ObstacleKind rightKind  = ObstacleKind::CACTUS_SMALL; // safe default

    for (const auto& o : _obs) {
        if (!o.active) continue;
        ++nActive;
        float edge = o.x + (float)_obsWidth(o.kind);
        if (edge > rightmost) {
            rightmost = edge;
            rightKind = o.kind;
        }
    }

    // Gate: spawn only when pipeline is empty OR a second slot is available
    // and the current rightmost obstacle is nearly on-screen.
    bool shouldSpawn = (nActive == 0) ||
                       (nActive < MAX_OBS &&
                        rightmost < (float)(SCREEN_W + MAX_GAP));
    if (!shouldSpawn) return;

    // Find a free slot.
    for (auto& o : _obs) {
        if (o.active) continue;

        // Place the new obstacle past the rightmost edge, with a random gap.
        float base = (rightmost < (float)SCREEN_W) ? (float)SCREEN_W : rightmost;
        o.x         = base + (float)random(MIN_GAP, MAX_GAP + 1);
        o.active    = true;
        o.animFrame = 0;
        o.animTimer = 0;

        // ── Type selection ────────────────────────────────────────────────────
        // Pterodactyls are gated behind score and cannot follow one another.
        // The back-to-back check uses the kind of the *rightmost* active
        // obstacle, which is the one the new spawn will follow.
        bool pteroEligible = (_score >= PTERO_MIN_SCORE) && !_isPtero(rightKind);

        if (pteroEligible &&
            random(PTERO_W_WEIGHT + CACTUS_W_WEIGHT) < PTERO_W_WEIGHT) {
            // Pterodactyl: equal chance of low (jump) or high (duck).
            o.kind = (random(2) == 0) ? ObstacleKind::PTERO_LOW
                                       : ObstacleKind::PTERO_HIGH;
        } else {
            // Cactus: 25 % large, 75 % small.
            o.kind = (random(4) == 0) ? ObstacleKind::CACTUS_LARGE
                                       : ObstacleKind::CACTUS_SMALL;
        }
        break;
    }
}

// ─── _checkCollision ─────────────────────────────────────────────────────────
//
// Single collision function for all obstacle types.
// Hitboxes are inset from the sprite bounds to keep gameplay fair.
bool DinoGame::_checkCollision(const Obstacle& o) const {
    // Dino hitbox — differs for standing vs ducking.
    int dx, dy, dw, dh;
    if (_isDucking) {
        // Ducking: snapped to ground, horizontally slightly narrower.
        dx = DINO_X + 3;
        dy = GROUND_Y - DUCK_H + 1;   // 45
        dw = DINO_W - 5;              // 11
        dh = DUCK_H - 2;              //  6
    } else {
        // Standing / jumping: inset 4 px left/right, 2 px top, 2 px bottom.
        dx = DINO_X + 4;
        dy = (int)_dinoY + 2;
        dw = DINO_W - 8;              //  8
        dh = DINO_H - 4;              // 12
    }

    // Obstacle hitbox.
    int ox = (int)o.x;
    int oy = _obsTopY(o.kind);
    int ow, oh;

    if (_isPtero(o.kind)) {
        // Pterodactyl: inset 2 px left/right, 1 px top/bottom.
        ox += 2;  ow = PTERO_W  - 4;
        oy += 1;  oh = PTERO_H  - 2;
    } else {
        // Cactus: inset 1 px left/right, 1 px bottom.
        ox += 1;  ow = _obsWidth(o.kind) - 2;
                  oh = CACTUS_H - 1;
    }

    return aabbOverlap(dx, dy, dw, dh, ox, oy, ow, oh);
}

// ─── _drawCloud ───────────────────────────────────────────────────────────────
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

            // Ducking is only valid on the ground and suppresses jump.
            _isDucking = wantDuck && _onGround;

            // ── Jump buffer ───────────────────────────────────────────────────
            if (jumpPressed)         _jumpBuffer = JUMP_BUFFER_FRAMES;
            if (_jumpBuffer  > 0)    _jumpBuffer--;

            // ── Coyote time ───────────────────────────────────────────────────
            if (_onGround) {
                _coyoteFrames = COYOTE_FRAMES;
            } else {
                if (_coyoteFrames > 0) _coyoteFrames--;
            }

            // ── Jump ──────────────────────────────────────────────────────────
            if (_jumpBuffer > 0 && _coyoteFrames > 0 && !_isDucking) {
                _dinoVY       = JUMP_VY;
                _onGround     = false;
                _coyoteFrames = 0;
                _jumpBuffer   = 0;
                SFX::jump(sound);
            }

            // ── Physics ───────────────────────────────────────────────────────
            _dinoVY += GRAVITY;
            _dinoY  += _dinoVY;
            const float groundPos = (float)(GROUND_Y - DINO_H);
            if (_dinoY >= groundPos) {
                _dinoY    = groundPos;
                _dinoVY   = 0.0f;
                _onGround = true;
            }

            // ── Speed ramp ────────────────────────────────────────────────────
            if (_speed < MAX_SPEED) _speed += SPEED_INC;

            // ── Move all obstacles; cull off-screen; animate pterodactyls ─────
            for (auto& o : _obs) {
                if (!o.active) continue;

                o.x -= _speed;

                // Cull once the right edge leaves the left side of the screen.
                if (o.x + (float)_obsWidth(o.kind) < 0.0f) {
                    o.active = false;
                    continue;
                }

                // Pterodactyl wing animation.
                if (_isPtero(o.kind)) {
                    if (++o.animTimer >= PTERO_ANIM_RATE) {
                        o.animTimer = 0;
                        o.animFrame ^= 1;
                    }
                }
            }

            // Spawn next obstacle into any free slot.
            _spawnObsIfNeeded();

            // ── Clouds (slow parallax) ─────────────────────────────────────────
            for (auto& c : _clouds) {
                c.x -= _speed * 0.25f;
                if (c.x < -22.0f) {
                    c.x = (float)(SCREEN_W + random(10, 40));
                    c.y = (int8_t)(12 + random(10));
                }
            }

            // ── Score ─────────────────────────────────────────────────────────
            _score++;
            if (_score > _hiScore) _hiScore = _score;

            if (_score % SCORE_MILESTONE == 0 && _score != _lastMilestone) {
                _lastMilestone = _score;
                _flashTimer    = FLASH_FRAMES;
                SFX::point(sound);
            }
            if (_flashTimer > 0) _flashTimer--;

            // ── Dino leg animation (toggle every 8 frames) ─────────────────────
            if (++_animTimer >= 8) {
                _animTimer = 0;
                _animFrame ^= 1;
            }
            _frameCnt++;

            // ── Collision detection ───────────────────────────────────────────
            for (auto& o : _obs) {
                if (o.active && _checkCollision(o)) {
                    _state = DinoState::DEAD;
                    SFX::death(sound);
                    break;
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

    // ── Clouds (background layer) ──────────────────────────────────────────────
    for (const auto& c : _clouds)
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
        int duckY = GROUND_Y - DUCK_H;
        disp.drawBitmap(DINO_X, duckY, 2, DUCK_H,
                        (_animFrame == 0) ? spr_duck1 : spr_duck2);
    } else if (!_onGround) {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, spr_run1);
    } else {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H,
                        (_animFrame == 0) ? spr_run1 : spr_run2);
    }

    // ── Obstacles (cacti and pterodactyls — same loop) ────────────────────────
    for (const auto& o : _obs) {
        if (!o.active) continue;
        const int ox = (int)o.x;
        const int oy = _obsTopY(o.kind);

        switch (o.kind) {
            case ObstacleKind::CACTUS_SMALL:
                disp.drawBitmap(ox, oy, 1, CACTUS_H, spr_cactus_s);
                break;
            case ObstacleKind::CACTUS_LARGE:
                disp.drawBitmap(ox, oy, 2, CACTUS_H, spr_cactus_l);
                break;
            case ObstacleKind::PTERO_LOW:
            case ObstacleKind::PTERO_HIGH:
                disp.drawBitmap(ox, oy, 2, PTERO_H,
                                (o.animFrame == 0) ? spr_ptero1 : spr_ptero2);
                break;
        }
    }

    // ── HUD: score ────────────────────────────────────────────────────────────
    char buf[12];
    disp.setFont(u8g2_font_6x10_tf);

    if (_flashTimer > 0) {
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

    // ── Game-over overlay ─────────────────────────────────────────────────────
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
bool DinoGame::isRunning()      const { return _running; }
const char* DinoGame::getName() const { return "Dino Run"; }
const uint8_t* DinoGame::getIcon() const { return nullptr; }