#include "SpaceInvadersGame.h"
#include "SpaceInvadersSprites.h"

// ═════════════════════════════════════════════════════════════════════════════
// SITitleScene
// ═════════════════════════════════════════════════════════════════════════════
void SITitleScene::onEnter(Console& ctx) { _frame = 0; }

void SITitleScene::update(Console& ctx, SceneManager& sm) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);
    }
}

void SITitleScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(12, 24, "SPACE INVADERS");
    ctx.drawHLine(0, 30, Console::W);
    
    ctx.drawBitmap(60, 36, 1, 8, spr_si_alien1);

    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(28, 58, "Press A to play");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SIPlayScene
// ═════════════════════════════════════════════════════════════════════════════
void SIPlayScene::onEnter(Console& ctx) {
    _data->score = 0;
    _data->lives = 3;
    _playerX = Console::W / 2 - 4;
    _respawnTimer = 0;
    _initLevel();
}

void SIPlayScene::_initLevel() {
    _swarmX = 10.0f;
    _swarmY = 12.0f;
    _swarmVX = 2.0f;
    _pbActive = false;
    _aliensAlive = ALIEN_ROWS * ALIEN_COLS;
    _moveDelay = 25;
    
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            _aliens[r][c] = true;
        }
    }
    for (auto& eb : _eBullets) eb.active = false;
}

void SIPlayScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.push(_pause, ctx);
        return;
    }

    if (_shakeFrames > 0) _shakeFrames--;

    // Handle Respawn Pause
    if (_respawnTimer > 0) {
        _respawnTimer--;
    } else {
        // Player Movement
        if (ctx.pressed(Btn::LEFT))  _playerX -= 2.0f;
        if (ctx.pressed(Btn::RIGHT)) _playerX += 2.0f;
        _playerX = gclamp(_playerX, 2.0f, (float)(Console::W - 10));

        // Player Shooting
        if (ctx.justPressed(Btn::A) && !_pbActive) {
            _pbActive = true;
            _pbX = _playerX + 3;
            _pbY = 52.0f;
            ctx.beep(1200, 20);
        }
    }

    // Player Movement
    if (ctx.pressed(Btn::LEFT))  _playerX -= 2.0f;
    if (ctx.pressed(Btn::RIGHT)) _playerX += 2.0f;
    _playerX = gclamp(_playerX, 2.0f, (float)(Console::W - 10));

    if (_pbActive) {
        _pbY -= 4.0f;
        if (_pbY < 0) _pbActive = false;
    }

    // Alien Movement (Step-based)
    if (++_moveTimer >= _moveDelay) {
        _moveTimer = 0;
        _animFrame ^= 1;
        _swarmX += _swarmVX;
        
        ctx.beep(100 + (_animFrame * 50), 10); // Classic march sound

        // Check bounds to drop
        float rightmost = 0;
        float leftmost = Console::W;
        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (_aliens[r][c]) {
                    float ax = _swarmX + c * 8;
                    if (ax > rightmost) rightmost = ax;
                    if (ax < leftmost) leftmost = ax;
                }
            }
        }

        if (rightmost > Console::W - 10 || leftmost < 2) {
            _swarmVX = -_swarmVX;
            _swarmY += 4.0f;
            
            // If aliens reach the player
            if (_swarmY + (ALIEN_ROWS * 8) > 56) {
                ctx.saveHiScore(_data->hiScore);
                sm.replace(_gameover, ctx);
                return;
            }
        }

        // Alien Shooting
        if (random(10) < 3) {
            for (auto& eb : _eBullets) {
                if (!eb.active) {
                    int c = random(ALIEN_COLS);
                    for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
                        if (_aliens[r][c]) {
                            eb.active = true;
                            eb.x = _swarmX + c * 8 + 2;
                            eb.y = _swarmY + r * 8 + 5;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // Enemy Bullets logic
    for (auto& eb : _eBullets) {
        if (eb.active) {
            eb.y += 2.0f;
            if (eb.y > Console::H) eb.active = false;
        }
    }

    _checkCollisions(ctx, sm);

    // Level Clear
    if (_aliensAlive == 0) {
        _initLevel();
        ctx.beep(1500, 100);
    }
}

void SIPlayScene::_checkCollisions(Console& ctx, SceneManager& sm) {
    Rect playerRect = {(int)_playerX, 56, 7, 5};

    // Check Player Bullet vs Aliens
    if (_pbActive) {
    Rect pbRect = {(int)_pbX, (int)_pbY, 1, 3}; // Skinny 1x3 bullet
    bool hit = false;
    for (int r = 0; r < ALIEN_ROWS && !hit; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            if (_aliens[r][c]) {
                // Alien hitbox (now 5x5, spaced 8px apart)
                Rect alienRect = {(int)(_swarmX + c * 8), (int)(_swarmY + r * 8), 5, 5};
                if (pbRect.overlaps(alienRect)) {
                        _aliens[r][c] = false;
                        _pbActive = false;
                        _aliensAlive--;
                        _data->score += 10;
                        if (_data->score > _data->hiScore) _data->hiScore = _data->score;
                        _moveDelay = max(2, 25 - ((ALIEN_ROWS * ALIEN_COLS - _aliensAlive) * 2));
                        hit = true;
                        ctx.sfxPoint();
                    }
                }
            }
        }
    }

    // Check Enemy Bullets vs Player
    if (_respawnTimer == 0) { // Only check if the player is vulnerable
        for (auto& eb : _eBullets) {
            if (eb.active) {
                Rect ebRect = {(int)eb.x, (int)eb.y, 1, 3};
                if (ebRect.overlaps(playerRect)) {
                    ctx.sfxDeath();
                    _shakeFrames = 15;
                    _data->lives--;

                    if (_data->lives < 0) {
                        // Out of lives, actual Game Over
                        ctx.saveHiScore(_data->hiScore);
                        sm.replace(_gameover, ctx);
                        return;
                    } else {
                        // Lost a life, trigger respawn
                        _respawnTimer = 60; // 2 seconds of freeze/invulnerability
                        _playerX = Console::W / 2 - 4; // Re-center player
                        _pbActive = false; // Kill active player bullet
                        for (auto& b : _eBullets) b.active = false; // Clear enemy bullets
                        return;
                    }
                }
            }
        }
    }
}

void SIPlayScene::draw(Console& ctx) {
    int ox = (_shakeFrames > 0) ? random(-2, 3) : 0;
    int oy = (_shakeFrames > 0) ? random(-2, 3) : 0;
    drawField(ctx);
}

void SIPlayScene::drawField(Console& ctx) const {
    // UI
    ctx.setFont(u8g2_font_5x7_tf);
    char buf[24];
    snprintf(buf, sizeof(buf), "SC:%u  HI:%u", (unsigned)_data->score, (unsigned)_data->hiScore);
    ctx.drawStr(2, 7, buf);
    
    // Draw Lives (Top Right)
    for (int i = 0; i < _data->lives; i++) {
        // Space them 9 pixels apart, starting from the right edge
        ctx.drawBitmap(Console::W - 8 - (i * 9), 2, 1, 5, spr_si_player);
    }
    
    ctx.drawHLine(0, 9, Console::W);

    // Player (Blink rapidly if respawning)
    if (_respawnTimer == 0 || (_respawnTimer / 4) % 2 == 0) {
        ctx.drawBitmap((int)_playerX, 56, 1, 5, spr_si_player);
    }

    // Player Bullet (skinny 1x3)
    if (_pbActive) ctx.drawBox((int)_pbX, (int)_pbY, 1, 3);

    // Enemy Bullets (skinny 1x3)
    for (const auto& eb : _eBullets) {
        if (eb.active) ctx.drawBox((int)eb.x, (int)eb.y, 1, 3);
    }

    // Aliens (height=5, spacing=8)
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            if (_aliens[r][c]) {
                int ax = (int)(_swarmX + c * 8);
                int ay = (int)(_swarmY + r * 8);
                const uint8_t* spr = (_animFrame == 0) ? spr_si_alien1 : spr_si_alien2;
                ctx.drawBitmap(ax, ay, 1, 5, spr);
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SIPauseScene & SIGameOverScene
// ═════════════════════════════════════════════════════════════════════════════
void SIPauseScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.pop(ctx);
    }
}

void SIPauseScene::draw(Console& ctx) {
    _play->drawField(ctx);
    ctx.setDrawColor(0);
    ctx.drawBox(34, 22, 60, 22);
    ctx.setDrawColor(1);
    ctx.drawFrame(34, 22, 60, 22);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(42, 37, "PAUSED");
}

void SIGameOverScene::onEnter(Console& ctx) { _frame = 0; }

void SIGameOverScene::update(Console& ctx, SceneManager& sm) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);
    }
}

void SIGameOverScene::draw(Console& ctx) {
    _play->drawField(ctx);
    ctx.setDrawColor(0);
    ctx.drawBox(20, 20, 88, 28);
    ctx.setDrawColor(1);
    ctx.drawFrame(20, 20, 88, 28);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(28, 36, "GAME OVER");
    
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(26, 45, "A to restart");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SpaceInvadersGame - OS Registration Hook
// ═════════════════════════════════════════════════════════════════════════════
void SpaceInvadersGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();

    _title.setPlayScene(&_play);
    _play.setData(&_data);
    _play.setPauseScene(&_pause);
    _play.setDeadScene(&_gameover);
    _pause.setPlayScene(&_play);
    _gameover.setData(&_data);
    _gameover.setPlayScene(&_play);

    _sm.replace(&_title, ctx);
}

void SpaceInvadersGame::onExit(Console& ctx) {
    ctx.saveHiScore(_data.hiScore);
}

void SpaceInvadersGame::update(Console& ctx) { _sm.update(ctx); }
void SpaceInvadersGame::draw(Console& ctx)   { _sm.draw(ctx); }

bool        SpaceInvadersGame::isRunning() const { return !_sm.empty(); }
const char* SpaceInvadersGame::getName()   const { return "Invaders"; }