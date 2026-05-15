#include "DinoGame.h"
#include "GameRegistry.h"
#include "DinoSprites.h"

// ─── DinoGame ────────────────────────────────────────────────────────────────
// Chrome-style endless runner.
//
// Controls:
//   UP / A    → Jump
//   DOWN      → Duck (ground only; fast fall in air)
//   MENU2 / B → Pause
//   MENU1     → Return to OS menu
//
// Persistent save data (NVS key "hi"):
//   Hi-score is loaded in onEnter() and saved immediately on death and on exit.
// ─────────────────────────────────────────────────────────────────────────────

// ═════════════════════════════════════════════════════════════════════════════
// DinoTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void DinoTitleScene::onEnter(Console& ctx) { 
    _frame = 0; 
}

void DinoTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    
    if (ctx.justPressed(Btn::MENU1)) { 
        sm.clear(ctx); 
        return; 
    }

    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // PlayScene
    }
}

void DinoTitleScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(36, 24, "DINO RUN");
    ctx.drawHLine(0, 30, Console::W);

    ctx.drawBitmap(10, 52 - 16, 2, 16, spr_run1); 

    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        int w = ctx.strWidth("Press A to play");
        ctx.drawStr((Console::W - w) / 2, 54, "Press A to play");
    }
}


// ═════════════════════════════════════════════════════════════════════════════
// DinoPlayScene
// ═════════════════════════════════════════════════════════════════════════════

/*static*/ int DinoPlayScene::_obsWidth(ObstacleKind k) {
    switch (k) {
        case ObstacleKind::CACTUS_LARGE: return LARGE_W;
        case ObstacleKind::PTERO_LOW:
        case ObstacleKind::PTERO_HIGH:   return PTERO_W;
        default:                         return SMALL_W;
    }
}

/*static*/ int DinoPlayScene::_obsTopY(ObstacleKind k) {
    switch (k) {
        case ObstacleKind::PTERO_LOW:  return PTERO_LOW_Y;
        case ObstacleKind::PTERO_HIGH: return PTERO_HIGH_Y;
        default:                       return GROUND_Y - CACTUS_H;
    }
}

/*static*/ bool DinoPlayScene::_isPtero(ObstacleKind k) {
    return k == ObstacleKind::PTERO_LOW || k == ObstacleKind::PTERO_HIGH;
}

void DinoPlayScene::onEnter(Console& ctx) {
    _initRound();
}

void DinoPlayScene::_initRound() {
    _dinoY         = (float)(GROUND_Y - DINO_H);
    _dinoVY        = 0.0f;
    _onGround      = true;
    _isDucking     = false;
    _coyoteFrames  = 0;
    _jumpBuffer    = 0;
    _data->score   = 0;
    _lastMilestone = 0;
    _flashTimer    = 0;
    _blinkTimer    = 100;
    _speed         = INIT_SPEED;
    _frameCnt      = 0;
    _animTimer     = 0;
    _animFrame     = 0;

    for (auto& o : _obs) o.active = false;
    for (auto& d : _dust) d.life = 0;
    for (auto& s : _sweat) s.life = 0;

    _clouds[0] = {{ 15.0f, 14.0f }};
    _clouds[1] = {{ 62.0f, 19.0f }};
    _clouds[2] = {{105.0f, 13.0f }};

    _spawnObsIfNeeded();
}

void DinoPlayScene::_spawnObsIfNeeded() {
    float        rightmost = -1.0f;
    uint8_t      nActive   = 0;
    ObstacleKind rightKind = ObstacleKind::CACTUS_SMALL;

    for (const auto& o : _obs) {
        if (!o.active) continue;
        ++nActive;
        float edge = o.x + (float)_obsWidth(o.kind);
        if (edge > rightmost) { 
            rightmost = edge; 
            rightKind = o.kind; 
        }
    }

    bool shouldSpawn = (nActive == 0) || (nActive < MAX_OBS && rightmost < (float)(SCREEN_W + MAX_GAP));
    if (!shouldSpawn) return;

    for (auto& o : _obs) {
        if (o.active) continue;

        float base  = (rightmost < (float)SCREEN_W) ? (float)SCREEN_W : rightmost;
        o.x         = base + (float)random(MIN_GAP, MAX_GAP + 1);
        o.active    = true;
        o.animFrame = 0;
        o.animTimer = 0;

        bool pteroEligible = (_data->score >= PTERO_MIN_SCORE) && !_isPtero(rightKind);

        if (pteroEligible && random(PTERO_W_WEIGHT + CACTUS_W_WEIGHT) < PTERO_W_WEIGHT) {
            o.kind = (random(2) == 0) ? ObstacleKind::PTERO_LOW : ObstacleKind::PTERO_HIGH;
        } else {
            o.kind = (random(4) == 0) ? ObstacleKind::CACTUS_LARGE : ObstacleKind::CACTUS_SMALL;
        }
        break;
    }
}

bool DinoPlayScene::_checkCollision(const Obstacle& o) const {
    Rect dino;
    if (_isDucking) dino = {DINO_X + 3, GROUND_Y - DUCK_H + 1, DINO_W - 5, DUCK_H - 2};
    else            dino = {DINO_X + 4, (int)_dinoY + 2, DINO_W - 8, DINO_H - 4};

    Rect obs {(int)o.x, _obsTopY(o.kind), _obsWidth(o.kind), _isPtero(o.kind) ? PTERO_H : CACTUS_H};

    if (_isPtero(o.kind)) obs = obs.inset(2, 1);
    else                  { obs = obs.inset(1, 0); obs.h -= 1; }

    return dino.overlaps(obs);
}

void DinoPlayScene::_drawCloud(Console& ctx, int x, int y) const {
    ctx.drawDisc(x + 4,  y + 5, 3);
    ctx.drawDisc(x + 9,  y + 3, 4);
    ctx.drawDisc(x + 15, y + 5, 3);
}

void DinoPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { 
        sm.clear(ctx); 
        return; 
    }
    
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::PAUSE);
        return;
    }

    bool jumpPressed = ctx.justPressed(Btn::UP) || ctx.justPressed(Btn::A);
    bool jumpHeld    = ctx.pressed(Btn::UP)     || ctx.pressed(Btn::A);
    bool wantDuck    = ctx.pressed(Btn::DOWN); 
    bool wasOnGround = _onGround;

    _isDucking = wantDuck && _onGround;

    if (jumpPressed)     _jumpBuffer = JUMP_BUFFER_FRAMES;
    if (_jumpBuffer > 0) _jumpBuffer--;

    if (_onGround) _coyoteFrames = COYOTE_FRAMES;
    else if (_coyoteFrames > 0) _coyoteFrames--;

    if (_jumpBuffer > 0 && _coyoteFrames > 0 && !_isDucking) {
        _dinoVY       = JUMP_VY;
        _onGround     = false;
        _coyoteFrames = 0;
        _jumpBuffer   = 0;
        ctx.sfxJump();
    }

    if (!_onGround) {
        if (_dinoVY < 0.0f && !jumpHeld) _dinoVY += GRAVITY * 1.5f; 
        if (wantDuck)                    _dinoVY += GRAVITY * 2.5f;
    }

    _dinoVY += GRAVITY;
    _dinoY  += _dinoVY;
    const float groundPos = (float)(GROUND_Y - DINO_H);
    
    if (_dinoY >= groundPos) {
        _dinoY    = groundPos;
        _dinoVY   = 0.0f;
        _onGround = true;
    }

    // Dust particles on landing
    if (!wasOnGround && _onGround) {
        for(int i = 0; i < MAX_DUST; i++) {
            _dust[i] = {{ (float)(DINO_X + random(2, 14)), (float)GROUND_Y }, (uint8_t)random(8, 16)};
        }
    }
    for(auto& d : _dust) {
        if (d.life > 0) { 
            d.pos.x -= (_speed * 0.4f); 
            d.pos.y -= 0.1f; 
            d.life--; 
        }
    }

    // Sweat particles at high speeds
    if (_speed > MAX_SPEED * 0.7f && random(15) == 0) {
        for(auto& s : _sweat) {
            if (s.life == 0) {
                s = {{ (float)(DINO_X + 2), _dinoY + 4.0f }, (uint8_t)random(10, 20)};
                break;
            }
        }
    }
    for(auto& s : _sweat) {
        if (s.life > 0) { 
            s.pos.x -= (_speed * 0.6f); 
            s.pos.y += 0.2f; 
            s.life--; 
        }
    }

    _speed = gclamp(_speed + SPEED_INC, INIT_SPEED, MAX_SPEED);
    _data->speed = _speed;

    for (auto& o : _obs) {
        if (!o.active) continue;
        o.x -= _speed;
        
        if (o.x + (float)_obsWidth(o.kind) < 0.0f) { 
            o.active = false; 
            continue; 
        }
        
        if (_isPtero(o.kind)) {
            if (++o.animTimer >= PTERO_ANIM_RATE) { 
                o.animTimer = 0; 
                o.animFrame ^= 1; 
            }
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

    _data->score++;
    if (_data->score > _data->hiScore) _data->hiScore = _data->score;
    
    if (_data->score > 0 && _data->score % SCORE_MILESTONE == 0 && _data->score != _lastMilestone) {
        _lastMilestone = _data->score;
        _flashTimer    = FLASH_FRAMES;
        ctx.sfxPoint();
    }
    
    if (_flashTimer > 0) _flashTimer--;
    
    if (_blinkTimer > 0) _blinkTimer--;
    else _blinkTimer = random(80, 200);

    if (++_animTimer >= 8) { 
        _animTimer = 0; 
        _animFrame ^= 1; 
    }
    _frameCnt++;

    for (auto& o : _obs) {
        if (o.active && _checkCollision(o)) {
            ctx.sfxDeath();
            ctx.saveHiScore(_data->hiScore);
            sm.emit(ctx, Event::GAME_OVER); 
            return;
        }
    }
}

void DinoPlayScene::draw(Console& ctx) {
    drawField(ctx, false);
}

void DinoPlayScene::drawField(Console& ctx, bool isDead) const {
    ctx.setCamera(nullptr); // Ensure UI/background reset

    bool isNight = (_data->score / 500) % 2 != 0;
    
    if (isNight) {
        ctx.setDrawColor(1);
        ctx.drawBox(0, 0, Console::W, Console::H); 
        ctx.setDrawColor(0); 
    } else {
        ctx.setDrawColor(0);
        ctx.drawBox(0, 0, Console::W, Console::H);
        ctx.setDrawColor(1);
    }

    // ── Celestial Body (Dimmed Sun/Moon) ──────────────────────────────────────
    // Moves extremely slowly leftwards across the sky
    int cx = Console::W - ((_frameCnt / 8) % (Console::W + 30));
    int cy = 16;
    
    ctx.setDrawColor(isNight ? 0 : 1);
    
    if (!isNight) {
        // Dimmed Sun (Stippled disc)
        for (int dy = -5; dy <= 5; dy++) {
            for (int dx = -5; dx <= 5; dx++) {
                if (dx*dx + dy*dy <= 25 && (dx + dy + cx + cy) % 2 == 0) {
                    ctx.drawPixel(cx + dx, cy + dy);
                }
            }
        }
        // Sun rays (dotted)
        ctx.drawPixel(cx, cy - 8);     ctx.drawPixel(cx, cy + 8);
        ctx.drawPixel(cx - 8, cy);     ctx.drawPixel(cx + 8, cy);
        ctx.drawPixel(cx - 6, cy - 6); ctx.drawPixel(cx + 6, cy + 6);
        ctx.drawPixel(cx - 6, cy + 6); ctx.drawPixel(cx + 6, cy - 6);
    } else {
        // Dimmed Moon (Stippled Crescent)
        for (int dy = -5; dy <= 5; dy++) {
            for (int dx = -5; dx <= 5; dx++) {
                // Main moon body
                if (dx*dx + dy*dy <= 25) {
                    // Cut out an offset circle to make a crescent
                    int cutX = dx + 2;
                    int cutY = dy - 2;
                    if (cutX*cutX + cutY*cutY > 25) {
                        if ((dx + dy + cx + cy) % 2 == 0) {
                            ctx.drawPixel(cx + dx, cy + dy);
                        }
                    }
                }
            }
        }
        
        // Add a few dimmed stars at night
        ctx.drawPixel(cx - 30, cy - 5);
        ctx.drawPixel(cx + 40, cy + 10);
        ctx.drawPixel(cx + 15, cy - 12);
    }

    ctx.setCamera(_camera);

    for (const auto& c : _clouds) {
        _drawCloud(ctx, c.pos.ix(), c.pos.iy());
    }

    ctx.drawHLine(0, GROUND_Y, SCREEN_W);
    int offset = (int)((float)_frameCnt * _speed) % 20;
    
    for (int x = -offset; x < SCREEN_W; x += 20) {
        ctx.drawHLine(x + 3,  GROUND_Y + 2, 6);
        ctx.drawHLine(x + 13, GROUND_Y + 4, 3);
    }

    for (const auto& d : _dust) {
        if (d.life > 0) ctx.drawPixel(d.pos.ix(), d.pos.iy());
    }
    for (const auto& s : _sweat) {
        if (s.life > 0) ctx.drawPixel(s.pos.ix(), s.pos.iy());
    }

    int dx = DINO_X;
    int dy = (int)_dinoY;

    if (isDead) {
        ctx.drawBitmap(dx, dy, 2, DINO_H, spr_dead);
    } else if (_isDucking) {
        ctx.drawBitmap(dx, GROUND_Y - DUCK_H, 2, DUCK_H, (_animFrame == 0) ? spr_duck1 : spr_duck2);
    } else if (!_onGround) {
        ctx.drawBitmap(dx, dy, 2, DINO_H, spr_run1);
    } else {
        ctx.drawBitmap(dx, dy, 2, DINO_H, (_animFrame == 0) ? spr_run1 : spr_run2);
    }

    // ── Blinking Hack (Draw over the eye pixel) ───────────────────────────────
    if (!isDead && !_isDucking && _blinkTimer < 4) {
        ctx.setDrawColor(isNight ? 1 : 0); 
        ctx.drawPixel(dx + 11, dy + 2);
        ctx.setDrawColor(isNight ? 0 : 1); 
    }

    for (const auto& o : _obs) {
        if (!o.active) continue;
        
        const int obx = (int)o.x;
        const int oby = _obsTopY(o.kind);
        
        switch (o.kind) {
            case ObstacleKind::CACTUS_SMALL: 
                ctx.drawBitmap(obx, oby, 1, CACTUS_H, spr_cactus_s); 
                break;
            case ObstacleKind::CACTUS_LARGE: 
                ctx.drawBitmap(obx, oby, 2, CACTUS_H, spr_cactus_l); 
                break;
            case ObstacleKind::PTERO_LOW:
            case ObstacleKind::PTERO_HIGH:
                ctx.drawBitmap(obx, oby, 2, PTERO_H, (o.animFrame == 0) ? spr_ptero1 : spr_ptero2);
                break;
        }
    }

    // ── Plain Text Score ──────────────────────────────────────────────────────
    ctx.setCamera(nullptr); // Unset camera for UI

    char buf[32];
    ctx.setFont(u8g2_font_6x10_tf);

    bool drawScore = true;
    if (_flashTimer > 0) drawScore = ((_flashTimer / 10) % 2 == 0);

    if (drawScore) {
        snprintf(buf, sizeof(buf), "HI:%05u  %05u", (unsigned)_data->hiScore, (unsigned)_data->score);
        ctx.setDrawColor(isNight ? 0 : 1);
        ctx.drawStr(34, 10, buf);
    }

    ctx.setDrawColor(1); // Force reset for standard UI elements
}


// ═════════════════════════════════════════════════════════════════════════════
// DinoPauseScene
// ═════════════════════════════════════════════════════════════════════════════

void DinoPauseScene::onEnter(Console& ctx) {}

void DinoPauseScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { 
        sm.emit(ctx, Event::QUIT); 
        return; 
    }

    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
    }
}

void DinoPauseScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);

    ctx.setDrawColor(0);
    ctx.drawBox(34, 22, 60, 22);
    ctx.setDrawColor(1);
    ctx.drawFrame(34, 22, 60, 22);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(42, 37, "PAUSED");
}


// ═════════════════════════════════════════════════════════════════════════════
// DinoDeadScene
// ═════════════════════════════════════════════════════════════════════════════

void DinoDeadScene::onEnter(Console& ctx) { 
    _frame = 0; 
    _camera->shake(12);

    // Explode debris outward based on the current speed
    float inheritSpeed = _data->speed * 0.5f;
    for (int i = 0; i < 15; i++) {
        _particles->spawnPixel(18.0f, 44.0f, 
            inheritSpeed + (random(-20, 30) / 10.0f), 
            (random(-50, -10) / 10.0f), 
            random(20, 40));
    }
}

void DinoDeadScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;

    if (ctx.justPressed(Btn::MENU1)) { 
        sm.emit(ctx, Event::QUIT); 
        return; 
    }
    
    // Prevent accidental instant-restarts by requiring a short delay (~1 second)
    if (_frame > 30) {
        if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
            sm.emit(ctx, Event::CUSTOM_1); // PlayScene
        }
    }
}

void DinoDeadScene::draw(Console& ctx) {
    if (_play) {
        _play->drawField(ctx, true);
    } else if (_sm) {
        _sm->drawUnder(ctx);
    }

    ctx.setCamera(_camera);
    _particles->draw(ctx);
    ctx.setCamera(nullptr);

    // Only draw the game over menu once the shake settles to give it impact
    if (_camera->getOffsetX() == 0 && _camera->getOffsetY() == 0) {
        ctx.setDrawColor(0);
        ctx.drawBox(18, 20, 92, 28);
        ctx.setDrawColor(1);
        ctx.drawFrame(18, 20, 92, 28);
        ctx.setFont(u8g2_font_7x13B_tf);
        ctx.drawStr(22, 36, "GAME  OVER");

        if ((_frame / 15) % 2 == 0) {
            ctx.setFont(u8g2_font_6x10_tf);
            ctx.drawStr(26, 46, "A to restart");
        }
    }
}


// ═════════════════════════════════════════════════════════════════════════════
// DinoGame - Framework Integration
// ═════════════════════════════════════════════════════════════════════════════

void DinoGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();

    _play.setData(&_data);
    _play.setEngine(&_camera, &_particles);

    _dead.setData(&_data);
    _dead.setEngine(&_camera, &_particles);
    _dead.setPlayScene(&_play);

    // Event Registry Mapping
    _sm.onEvent(Event::QUIT,      SceneManager::CLEAR);
    _sm.onEvent(Event::PAUSE,     SceneManager::PUSH, &_pause);
    _sm.onEvent(Event::RESUME,    SceneManager::POP);
    _sm.onEvent(Event::GAME_OVER, SceneManager::REPLACE, &_dead);
    _sm.onEvent(Event::CUSTOM_1,  SceneManager::REPLACE, &_play); // Start/Restart Game

    _sm.replace(&_title, ctx); 
}

void DinoGame::onExit(Console& ctx) {
    ctx.saveHiScore(_data.hiScore);
}

void DinoGame::update(Console& ctx, float dt) { 
    _camera.update();
    _particles.update();
    _sm.update(ctx, dt); 
}

void DinoGame::draw(Console& ctx) { 
    _sm.draw(ctx); 
}

bool           DinoGame::isRunning() const { return !_sm.empty(); }
const char*    DinoGame::getName()   const { return "Dino Run"; }
const uint8_t* DinoGame::getIcon()   const { return dinogame_icon; }
const uint8_t* DinoGame::getCoverArt() const { return spr_dino_cover; }

REGISTER_GAME(DinoGame);
