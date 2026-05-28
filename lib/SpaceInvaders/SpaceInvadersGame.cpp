#include "SpaceInvadersGame.h"
#include "GameRegistry.h"
#include "SpaceInvadersSprites.h"

// ═════════════════════════════════════════════════════════════════════════════
// SITitleScene
// ═════════════════════════════════════════════════════════════════════════════
void SITitleScene::onEnter(Console& ctx) { _frame = 0; }

void SITitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // PlayScene
    }
}

void SITitleScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(12, 24, "SPACE INVADERS");
    ctx.drawHLine(0, 30, Console::W);
    
    ctx.drawBitmap(60, 36, 1, 8, spr_si_alien1);

    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStrCentered(58, "Press A to play");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SIPlayScene
// ═════════════════════════════════════════════════════════════════════════════
void SIPlayScene::onEnter(Console& ctx) {
    _data->score = 0;
    _data->lives = 3;
    _respawnTimer = 0;
    _wave = 0;
    
    _player = Sprite(Console::W / 2 - 4, 56, 7, 5, spr_si_player);
    _pb = Sprite(0, 0, 1, 3, nullptr); 
    _pb.active = false;

    for (int i = 0; i < MAX_EBULLETS; i++) {
        _eBullets[i] = Sprite(0, 0, 1, 3, nullptr);
        _eBullets[i].active = false;
    }

    for (int i = 0; i < MAX_STARS; i++) {
        _stars[i].x = (float)random(Console::W);
        _stars[i].y = (float)random(Console::H);
        _stars[i].speed = 0.5f + (random(10) / 10.0f);
    }

    _initLevel();
}

void SIPlayScene::_initLevel() {
    _swarmX = 10.0f;
    // Lower start position based on wave, capped to max depth
    _swarmY = min(30.0f, 12.0f + (_wave * 4.0f));
    _swarmVX = 2.0f;
    _pb.active = false;
    _aliensAlive = ALIEN_ROWS * ALIEN_COLS;
    _moveDelay = max(10, 25 - (_wave * 2));
    
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            _aliens[r][c] = Sprite(_swarmX + c * 8, _swarmY + r * 8, 5, 5, spr_si_alien1);
            _aliens[r][c].active = true;
        }
    }
    for (auto& eb : _eBullets) eb.active = false;
}

void SIPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::PAUSE);
        return;
    }

    // Handle Respawn Pause (Invulnerability phase)
    if (_respawnTimer > 0) {
        _respawnTimer--;
    } 
    
    // Player Movement (Allowed even while respawning/invulnerable)
    if (ctx.pressed(Btn::LEFT))  _player.x -= 2.0f;
    if (ctx.pressed(Btn::RIGHT)) _player.x += 2.0f;
    _player.x = gclamp(_player.x, 2.0f, (float)(Console::W - 10));

    // Player Shooting (Allowed even while respawning)
    if (ctx.justPressed(Btn::A) && !_pb.active) {
        _pb.active = true;
        _pb.x = _player.x + 3;
        _pb.y = 52.0f;
        ctx.beep(1200, 20);
    }

    if (_pb.active) {
        _pb.y -= 4.0f;
        if (_pb.y < 0) _pb.active = false;
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
                if (_aliens[r][c].active) {
                    _aliens[r][c].x = _swarmX + c * 8;
                    float ax = _aliens[r][c].x;
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
                ctx.sfxDeath();
                if (_camera) _camera->shake(15);
                ctx.saveHiScore(_data->hiScore);
                sm.emit(ctx, Event::GAME_OVER);
                return;
            }
        }
        
        // Sync Y pos
        for (int r = 0; r < ALIEN_ROWS; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (_aliens[r][c].active) {
                    _aliens[r][c].y = _swarmY + r * 8;
                    _aliens[r][c].bitmap = (_animFrame == 0) ? spr_si_alien1 : spr_si_alien2;
                }
            }
        }

        // Alien Shooting
        if (random(10) < 3) {
            for (auto& eb : _eBullets) {
                if (!eb.active) {
                    int c = random(ALIEN_COLS);
                    for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
                        if (_aliens[r][c].active) {
                            eb.active = true;
                            eb.x = _aliens[r][c].x + 2;
                            eb.y = _aliens[r][c].y + 5;
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

    // Update Stars
    for (int i = 0; i < MAX_STARS; i++) {
        _stars[i].y += _stars[i].speed;
        if (_stars[i].y > Console::H) {
            _stars[i].y = 0;
            _stars[i].x = (float)random(Console::W);
        }
    }

    _checkCollisions(ctx, sm);

    // Level Clear
    if (_aliensAlive == 0) {
        _wave++;
        _initLevel();
        ctx.beep(1500, 100);
    }
}

void SIPlayScene::_checkCollisions(Console& ctx, SceneManager& sm) {
    // Check Player Bullet vs Aliens
    if (_pb.active) {
        bool hit = false;
        for (int r = 0; r < ALIEN_ROWS && !hit; r++) {
            for (int c = 0; c < ALIEN_COLS; c++) {
                if (_aliens[r][c].active) {
                    if (_pb.collidesWith(_aliens[r][c])) {
                        _aliens[r][c].active = false;
                        _pb.active = false;
                        _aliensAlive--;
                        
                        // Row-based scoring: Top row 30, Middle 20, Bottom 10
                        int points = 10;
                        if (r == 0) points = 30;
                        else if (r == 1) points = 20;
                        
                        _data->score += points;
                        if (_data->score > _data->hiScore) _data->hiScore = _data->score;
                        _moveDelay = max(2, 25 - (_wave * 2) - ((ALIEN_ROWS * ALIEN_COLS - _aliensAlive) * 2));
                        hit = true;
                        ctx.sfxPoint();

                        // Explosion particles
                        if (_particles) {
                            for (int i = 0; i < 6; i++) {
                                _particles->spawnPixel(
                                    _aliens[r][c].x + 2.0f, 
                                    _aliens[r][c].y + 2.0f, 
                                    (random(-20, 20) / 10.0f), 
                                    (random(-20, 20) / 10.0f), 
                                    random(10, 20)
                                );
                            }
                        }
                    }
                }
            }
        }
    }

    // Check Enemy Bullets vs Player
    if (_respawnTimer == 0) { // Only check if the player is vulnerable
        for (auto& eb : _eBullets) {
            if (eb.active) {
                if (eb.collidesWith(_player)) {
                    ctx.sfxDeath();
                    if (_camera) _camera->shake(15);
                    _data->lives--;

                    if (_data->lives < 0) {
                        // Out of lives, actual Game Over
                        ctx.saveHiScore(_data->hiScore);
                        sm.emit(ctx, Event::GAME_OVER);
                        return;
                    } else {
                        // Lost a life, trigger respawn
                        _respawnTimer = 60; // 2 seconds of freeze/invulnerability
                        _player.x = Console::W / 2 - 4; // Re-center player
                        _pb.active = false; // Kill active player bullet
                        for (auto& b : _eBullets) b.active = false; // Clear enemy bullets
                        return;
                    }
                }
            }
        }
    }
}

void SIPlayScene::draw(Console& ctx) {
    drawField(ctx);
}

void SIPlayScene::drawField(Console& ctx) const {
    ctx.setCamera(nullptr); // Ensure UI has no camera offset

    // Draw Stars
    ctx.setDrawColor(1);
    for (int i = 0; i < MAX_STARS; i++) {
        ctx.drawPixel((int)_stars[i].x, (int)_stars[i].y);
    }

    // UI
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawPrintf(2, 7, "SC:%u  HI:%u", (unsigned)_data->score, (unsigned)_data->hiScore);
    
    // Draw Lives (Top Right)
    for (int i = 0; i < _data->lives; i++) {
        // Space them 9 pixels apart, starting from the right edge
        ctx.drawBitmap(Console::W - 8 - (i * 9), 2, 1, 5, spr_si_player);
    }
    
    ctx.drawHLine(0, 9, Console::W);

    ctx.setCamera(_camera); // Apply camera for game world

    if (_particles) _particles->draw(ctx);

    // Player (Blink rapidly if respawning)
    if (_respawnTimer == 0 || (_respawnTimer / 4) % 2 == 0) {
        _player.draw(ctx);
    }

    // Player Bullet (skinny 1x3)
    if (_pb.active) ctx.drawBox((int)_pb.x, (int)_pb.y, 1, 3);

    // Enemy Bullets (skinny 1x3)
    for (const auto& eb : _eBullets) {
        if (eb.active) ctx.drawBox((int)eb.x, (int)eb.y, 1, 3);
    }

    // Aliens 
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            _aliens[r][c].draw(ctx);
        }
    }

    ctx.setCamera(nullptr); // Clean up
}

// ═════════════════════════════════════════════════════════════════════════════
// SIPauseScene & SIGameOverScene
// ═════════════════════════════════════════════════════════════════════════════
void SIPauseScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
    }
}

void SIPauseScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    ctx.setDrawColor(0);
    ctx.drawBox(34, 22, 60, 22);
    ctx.setDrawColor(1);
    ctx.drawFrame(34, 22, 60, 22);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(42, 37, "PAUSED");
}

void SIGameOverScene::onEnter(Console& ctx) { _frame = 0; }

void SIGameOverScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    
    if (_frame > 30) {
        if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
            ctx.sfxMenuEnter();
            sm.emit(ctx, Event::CUSTOM_1); // PlayScene
        }
    }
}

void SIGameOverScene::draw(Console& ctx) {
    if (_play) {
        _play->drawField(ctx);
    } else if (_sm) {
        _sm->drawUnder(ctx);
    }
    ctx.setDrawColor(0);
    ctx.drawBox(20, 20, 88, 28);
    ctx.setDrawColor(1);
    ctx.drawFrame(20, 20, 88, 28);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(28, 36, "GAME OVER");
    
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStrCentered(45, "A to restart");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// SpaceInvadersGame - OS Registration Hook
// ═════════════════════════════════════════════════════════════════════════════
void SpaceInvadersGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();

    _play.setData(&_data);
    _play.setEngine(&_camera, &_particles);
    
    _gameover.setData(&_data);
    _gameover.setPlayScene(&_play);
    _gameover.setEngine(&_particles);

    // Wire standard PAUSE and GAME_OVER events
    useDefaultEvents(&_pause, &_gameover);
    
    // Custom events
    _sm.onEvent(Event::CUSTOM_1, SceneManager::REPLACE, &_play); // Start/Restart Game

    _sm.replace(&_title, ctx);
}

const char* SpaceInvadersGame::getName()   const { return "Invaders"; }
const uint8_t* SpaceInvadersGame::getCoverArt() const { return spr_si_cover; }

REGISTER_GAME(SpaceInvadersGame);
