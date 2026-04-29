#include "DinoGame.h"
#include "DinoSprites.h"   // all PROGMEM bitmaps for this game
#include <U8g2lib.h>

// ─── Per-kind static queries ─────────────────────────────────────────────────

/*static*/ int DinoGame::_obsWidth(ObstacleKind k) {
    switch (k) {
        case ObstacleKind::CACTUS_LARGE: return LARGE_W;
        case ObstacleKind::PTERO_LOW:
        case ObstacleKind::PTERO_HIGH:   return PTERO_W;
        default:                          return SMALL_W;
    }
}

/*static*/ int DinoGame::_obsTopY(ObstacleKind k) {
    switch (k) {
        case ObstacleKind::PTERO_LOW:  return PTERO_LOW_Y;
        case ObstacleKind::PTERO_HIGH: return PTERO_HIGH_Y;
        default:                        return GROUND_Y - CACTUS_H;
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

    _clouds[0] = {{ 15.0f, 14.0f }};
    _clouds[1] = {{ 62.0f, 19.0f }};
    _clouds[2] = {{105.0f, 13.0f }};

    _spawnObsIfNeeded();
}

// ─── _spawnObsIfNeeded ────────────────────────────────────────────────────────
void DinoGame::_spawnObsIfNeeded() {
    float        rightmost = -1.0f;
    uint8_t      nActive   = 0;
    ObstacleKind rightKind = ObstacleKind::CACTUS_SMALL;

    for (const auto& o : _obs) {
        if (!o.active) continue;
        ++nActive;
        float edge = o.x + (float)_obsWidth(o.kind);
        if (edge > rightmost) { rightmost = edge; rightKind = o.kind; }
    }

    bool shouldSpawn = (nActive == 0) ||
                       (nActive < MAX_OBS && rightmost < (float)(SCREEN_W + MAX_GAP));
    if (!shouldSpawn) return;

    for (auto& o : _obs) {
        if (o.active) continue;

        float base = (rightmost < (float)SCREEN_W) ? (float)SCREEN_W : rightmost;
        o.x         = base + (float)random(MIN_GAP, MAX_GAP + 1);
        o.active    = true;
        o.animFrame = 0;
        o.animTimer = 0;

        bool pteroEligible = (_score >= PTERO_MIN_SCORE) && !_isPtero(rightKind);

        if (pteroEligible && random(PTERO_W_WEIGHT + CACTUS_W_WEIGHT) < PTERO_W_WEIGHT) {
            o.kind = (random(2) == 0) ? ObstacleKind::PTERO_LOW : ObstacleKind::PTERO_HIGH;
        } else {
            o.kind = (random(4) == 0) ? ObstacleKind::CACTUS_LARGE : ObstacleKind::CACTUS_SMALL;
        }
        break;
    }
}

// ─── _checkCollision ─────────────────────────────────────────────────────────
bool DinoGame::_checkCollision(const Obstacle& o) const {
    Rect dino;
    if (_isDucking) {
        dino = {DINO_X + 3, GROUND_Y - DUCK_H + 1, DINO_W - 5, DUCK_H - 2};
    } else {
        dino = {DINO_X + 4, (int)_dinoY + 2, DINO_W - 8, DINO_H - 4};
    }

    Rect obs {(int)o.x, _obsTopY(o.kind),
              _obsWidth(o.kind), _isPtero(o.kind) ? PTERO_H : CACTUS_H};

    if (_isPtero(o.kind)) {
        obs = obs.inset(2, 1);
    } else {
        obs = obs.inset(1, 0);
        obs.h -= 1;
    }

    return dino.overlaps(obs);
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

        case DinoState::RUNNING: {
            _isDucking = wantDuck && _onGround;

            if (jumpPressed)       _jumpBuffer = JUMP_BUFFER_FRAMES;
            if (_jumpBuffer > 0)   _jumpBuffer--;

            if (_onGround) {
                _coyoteFrames = COYOTE_FRAMES;
            } else {
                if (_coyoteFrames > 0) _coyoteFrames--;
            }

            if (_jumpBuffer > 0 && _coyoteFrames > 0 && !_isDucking) {
                _dinoVY       = JUMP_VY;
                _onGround     = false;
                _coyoteFrames = 0;
                _jumpBuffer   = 0;
                SFX::jump(sound);
            }

            _dinoVY += GRAVITY;
            _dinoY  += _dinoVY;
            const float groundPos = (float)(GROUND_Y - DINO_H);
            if (_dinoY >= groundPos) {
                _dinoY    = groundPos;
                _dinoVY   = 0.0f;
                _onGround = true;
            }

            _speed = gclamp(_speed + SPEED_INC, INIT_SPEED, MAX_SPEED);

            for (auto& o : _obs) {
                if (!o.active) continue;
                o.x -= _speed;
                if (o.x + (float)_obsWidth(o.kind) < 0.0f) { o.active = false; continue; }
                if (_isPtero(o.kind)) {
                    if (++o.animTimer >= PTERO_ANIM_RATE) { o.animTimer = 0; o.animFrame ^= 1; }
                }
            }

            _spawnObsIfNeeded();

            for (auto& c : _clouds) {
                c.pos.x -= _speed * 0.25f;
                if (c.pos.x < -22.0f) {
                    c.pos.x = (float)(SCREEN_W + random(10, 40));
                    c.pos.y = (float)(12 + random(10));
                }
            }

            _score++;
            if (_score > _hiScore) _hiScore = _score;
            if (_score % SCORE_MILESTONE == 0 && _score != _lastMilestone) {
                _lastMilestone = _score;
                _flashTimer    = FLASH_FRAMES;
                SFX::point(sound);
            }
            if (_flashTimer > 0) _flashTimer--;

            if (++_animTimer >= 8) { _animTimer = 0; _animFrame ^= 1; }
            _frameCnt++;

            for (auto& o : _obs) {
                if (o.active && _checkCollision(o)) {
                    _state = DinoState::DEAD;
                    SFX::death(sound);
                    break;
                }
            }
            break;
        }

        case DinoState::DEAD:
            _isDucking = false;
            if (jumpPressed) _initRound();
            break;
    }

    if (menuPressed) _running = false;
}

// ─── draw ────────────────────────────────────────────────────────────────────
void DinoGame::draw(U8G2& disp) {

    for (const auto& c : _clouds)
        _drawCloud(disp, c.pos.ix(), c.pos.iy());

    disp.drawHLine(0, GROUND_Y, SCREEN_W);

    int offset = (int)((float)_frameCnt * _speed) % 20;
    for (int x = -offset; x < SCREEN_W; x += 20) {
        disp.drawHLine(x + 3,  GROUND_Y + 2, 6);
        disp.drawHLine(x + 13, GROUND_Y + 4, 3);
    }

    if (_state == DinoState::DEAD) {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, spr_dead);
    } else if (_isDucking) {
        int duckY = GROUND_Y - DUCK_H;
        disp.drawBitmap(DINO_X, duckY, 2, DUCK_H, (_animFrame == 0) ? spr_duck1 : spr_duck2);
    } else if (!_onGround) {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, spr_run1);
    } else {
        disp.drawBitmap(DINO_X, (int)_dinoY, 2, DINO_H, (_animFrame == 0) ? spr_run1 : spr_run2);
    }

    for (const auto& o : _obs) {
        if (!o.active) continue;
        const int ox = (int)o.x;
        const int oy = _obsTopY(o.kind);
        switch (o.kind) {
            case ObstacleKind::CACTUS_SMALL: disp.drawBitmap(ox, oy, 1, CACTUS_H, spr_cactus_s); break;
            case ObstacleKind::CACTUS_LARGE: disp.drawBitmap(ox, oy, 2, CACTUS_H, spr_cactus_l); break;
            case ObstacleKind::PTERO_LOW:
            case ObstacleKind::PTERO_HIGH:
                disp.drawBitmap(ox, oy, 2, PTERO_H, (o.animFrame == 0) ? spr_ptero1 : spr_ptero2);
                break;
        }
    }

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
bool DinoGame::isRunning()         const { return _running; }
const char* DinoGame::getName()    const { return "Dino Run"; }
const uint8_t* DinoGame::getIcon() const { return nullptr; }