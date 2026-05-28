#include "BrickBreakerGame.h"
#include "BrickBreakerSprites.h"
#include "GameRegistry.h"
#include "CommonScreens.h"
#include <Arduino.h>

// ─── Constants ──────────────────────────────────────────────────────────────
static constexpr uint32_t MESSAGE_DURATION = 120; // Frames

// ─── Helper Functions ───────────────────────────────────────────────────────
static bool rectIntersect(float r1x, float r1y, float r1w, float r1h, float r2x, float r2y, float r2w, float r2h) {
    return (r1x < r2x + r2w && r1x + r1w > r2x && r1y < r2y + r2h && r1y + r1h > r2y);
}

// ─── BBTitleScene ───────────────────────────────────────────────────────────
void BBTitleScene::onEnter(Console& ctx) {
    _frame = 0;
}

void BBTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Start Game
    }
    if (ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuBack();
        sm.clear(ctx);
    }
}

void BBTitleScene::draw(Console& ctx) {
    Screens::drawTitle(ctx, "BRICK BREAKER");
}

// ─── BBPlayScene ────────────────────────────────────────────────────────────

void BBPlayScene::onEnter(Console& ctx) {
    _data->score = 0;
    _data->lives = 3;
    _data->level = 1;
    _generateLevel();
    _resetBall(true);
    
    // Clear powerups
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        _powerUps[i].active = false;
    }
}

void BBPlayScene::_generateLevel() {
    _bricksLeft = 0;
    _padX = (Console::W - _padW) / 2.0f;
    _stickyPaddle = false;

    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            // Leave some empty space based on level
            if (random(10) < 2) {
                _bricks[r][c].active = false;
                continue;
            }

            _bricks[r][c].active = true;
            _bricksLeft++;
            
            int rnd = random(100);
            if (rnd < 10 + _data->level * 2) {
                _bricks[r][c].type = BrickType::HARD;
                _bricks[r][c].hp = 2;
            } else if (rnd < 15 + _data->level * 3 && r < 2) {
                _bricks[r][c].type = BrickType::SOLID;
                _bricks[r][c].hp = -1; // Indestructible
                _bricksLeft--; // Doesn't count towards completion
            } else {
                _bricks[r][c].type = BrickType::NORMAL;
                _bricks[r][c].hp = 1;
            }
        }
    }
    _msgTimer = MESSAGE_DURATION;
    snprintf(_msg, sizeof(_msg), "Level %d", _data->level);
}

void BBPlayScene::_resetBall(bool serve) {
    for (int i = 1; i < MAX_BALLS; ++i) _balls[i].active = false;
    
    _balls[0].active = true;
    _balls[0].fireball = false;
    
    if (serve) {
        _balls[0].sticky = true;
        _balls[0].stuckOffset = 0;
        _balls[0].x = _padX + _padW / 2.0f;
        _balls[0].y = PAD_Y - 2;
    } else {
        _balls[0].sticky = false;
        _balls[0].x = Console::W / 2.0f;
        _balls[0].y = Console::H / 2.0f;
        _balls[0].vx = (random(2) == 0 ? 1 : -1) * INIT_SPEED * 0.7f;
        _balls[0].vy = -INIT_SPEED;
    }
    _padW = 20.0f;
    _combo = 1;
}

void BBPlayScene::_normalizeBallVelocity(BBBall& b, float speedTarget) {
    float mag = sqrtf(b.vx * b.vx + b.vy * b.vy);
    if (mag > 0.001f) {
        b.vx = (b.vx / mag) * speedTarget;
        b.vy = (b.vy / mag) * speedTarget;
    }
}

void BBPlayScene::_spawnPowerUp(float x, float y) {
    if (random(100) > 20) return; // 20% chance

    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (!_powerUps[i].active) {
            _powerUps[i].active = true;
            _powerUps[i].x = x;
            _powerUps[i].y = y;
            _powerUps[i].type = static_cast<PowerUpType>(random(6));
            break;
        }
    }
}

void BBPlayScene::_applyPowerUp(PowerUpType type, Console& ctx) {
    ctx.sfxPoint(); // generic powerup sound
    switch (type) {
        case PowerUpType::EXPAND:
            _padW = min(36.0f, _padW + 8.0f);
            break;
        case PowerUpType::SHRINK:
            _padW = max(12.0f, _padW - 6.0f);
            break;
        case PowerUpType::CATCH:
            _stickyPaddle = true;
            break;
        case PowerUpType::MULTIBALL: {
            int activeBalls = 0;
            int sourceBallIdx = 0;
            for (int i = 0; i < MAX_BALLS; ++i) {
                if (_balls[i].active) {
                    activeBalls++;
                    sourceBallIdx = i;
                }
            }
            if (activeBalls > 0) {
                for (int i = 0; i < MAX_BALLS; ++i) {
                    if (!_balls[i].active) {
                        _balls[i] = _balls[sourceBallIdx];
                        _balls[i].vx = -_balls[sourceBallIdx].vx + (random(-10, 10) * 0.05f);
                        _balls[i].vy = _balls[sourceBallIdx].vy + (random(-10, 10) * 0.05f);
                        _normalizeBallVelocity(_balls[i], INIT_SPEED);
                    }
                }
            }
            break;
        }
        case PowerUpType::FIREBALL:
            for (int i = 0; i < MAX_BALLS; ++i) {
                if (_balls[i].active) _balls[i].fireball = true;
            }
            break;
        case PowerUpType::LIFE:
            _data->lives++;
            break;
    }
}

bool BBPlayScene::_checkBallBrick(BBBall& ball, BBBrick& brick, int bx, int by, Console& ctx) {
    if (!brick.active) return false;

    float brX = GRID_OX + bx * BRICK_W;
    float brY = GRID_OY + by * BRICK_H;

    // AABB collision
    if (rectIntersect(ball.x - 1, ball.y - 1, 2, 2, brX, brY, BRICK_W, BRICK_H)) {
        // Determine bounce direction
        float cX = ball.x;
        float cY = ball.y;
        float bcX = brX + BRICK_W / 2.0f;
        float bcY = brY + BRICK_H / 2.0f;
        
        if (!ball.fireball || brick.type == BrickType::SOLID) {
            if (abs(cX - bcX) / BRICK_W > abs(cY - bcY) / BRICK_H) {
                ball.vx = -ball.vx;
            } else {
                ball.vy = -ball.vy;
            }
        }

        if (brick.type != BrickType::SOLID) {
            if (ball.fireball) {
                brick.hp = 0;
                if (_camera) _camera->shake(2);
            } else {
                brick.hp--;
                if (brick.type == BrickType::HARD && brick.hp > 0) {
                    if (_camera) _camera->shake(1);
                }
            }

            // Speed up the ball slightly upon hitting a brick
            float speedSq = ball.vx*ball.vx + ball.vy*ball.vy;
            if (speedSq > 0.001f && speedSq < MAX_SPEED * MAX_SPEED) {
                float speed = sqrt(speedSq);
                _normalizeBallVelocity(ball, min((float)MAX_SPEED, speed + 0.05f));
            }

            if (brick.hp <= 0) {
                brick.active = false;
                _bricksLeft--;
                int baseScore = (brick.type == BrickType::HARD) ? 20 : 10;
                _data->score += baseScore * _combo;
                _combo = min(10, _combo + 1); // Cap combo at 10x
                ctx.updateHiScore(_data->score);
                _spawnPowerUp(brX + BRICK_W / 2.0f, brY + BRICK_H / 2.0f);
                if (_particles) {
                    for(int p=0; p<6; p++) {
                        _particles->spawnPixel(brX + BRICK_W / 2.0f, brY + BRICK_H / 2.0f, random(-20, 20)*0.1f, random(-20, 20)*0.1f, random(10, 20));
                    }
                }
            }
        }
        ctx.beep(400, 10);
        return true;
    }
    return false;
}

void BBPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) {
        sm.clear(ctx);
        return;
    }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        sm.emit(ctx, Event::PAUSE);
        return;
    }

    if (_msgTimer > 0) _msgTimer--;
    if (_levelClearPause) {
        if (_clearTimer > 0) {
            _clearTimer--;
        } else {
            _levelClearPause = false;
            _data->level++;
            _generateLevel();
            _resetBall(true);
        }
        return;
    }

    // Paddle Movement
    if (ctx.pressed(Btn::LEFT))  _padX -= _padSpeed;
    if (ctx.pressed(Btn::RIGHT)) _padX += _padSpeed;
    
    // Clamp paddle
    if (_padX < 0) _padX = 0;
    if (_padX + _padW > Console::W) _padX = Console::W - _padW;

    // Launch Ball
    if (ctx.justPressed(Btn::A)) {
        for (int i = 0; i < MAX_BALLS; ++i) {
            if (_balls[i].active && _balls[i].sticky) {
                _balls[i].sticky = false;
                _balls[i].vy = -INIT_SPEED;
                _balls[i].vx = ((_balls[i].x - (_padX + _padW/2.0f)) / (_padW/2.0f)) * INIT_SPEED;
                _normalizeBallVelocity(_balls[i], INIT_SPEED);
            }
        }
    }

    // Update Balls
    bool anyBallActive = false;
    for (int i = 0; i < MAX_BALLS; ++i) {
        if (!_balls[i].active) continue;
        anyBallActive = true;

        if (_balls[i].sticky) {
            _balls[i].x = _padX + _padW / 2.0f + _balls[i].stuckOffset;
            _balls[i].y = PAD_Y - 2;
            continue;
        }

        _balls[i].x += _balls[i].vx;
        _balls[i].y += _balls[i].vy;

        // Fireball trail
        if (_balls[i].fireball && _particles) {
            if (random(100) < 60) { // 60% chance per frame to leave a spark
                _particles->spawnPixel(_balls[i].x + 1, _balls[i].y + 1, random(-10, 10)*0.02f, random(-10, 10)*0.02f, random(10, 20));
            }
        }

        // Wall collisions
        if (_balls[i].x <= 0) { _balls[i].x = 0; _balls[i].vx = -_balls[i].vx; ctx.beep(300, 10); }
        if (_balls[i].x >= Console::W - 1) { _balls[i].x = Console::W - 1; _balls[i].vx = -_balls[i].vx; ctx.beep(300, 10); }
        if (_balls[i].y <= 0) { _balls[i].y = 0; _balls[i].vy = -_balls[i].vy; ctx.beep(300, 10); }

        // Paddle collision
        if (_balls[i].vy > 0 && _balls[i].y >= PAD_Y - 2 && _balls[i].y <= PAD_Y + 2) {
            if (_balls[i].x >= _padX - 2 && _balls[i].x <= _padX + _padW + 2) {
                _balls[i].y = PAD_Y - 2;
                _combo = 1; // Reset combo
                
                if (_stickyPaddle) {
                    _balls[i].sticky = true;
                    _balls[i].stuckOffset = _balls[i].x - (_padX + _padW / 2.0f);
                } else {
                    float hitDist = _balls[i].x - (_padX + _padW / 2.0f);
                    _balls[i].vx = (hitDist / (_padW / 2.0f)) * MAX_SPEED * 0.8f;
                    _balls[i].vy = -INIT_SPEED;
                    _normalizeBallVelocity(_balls[i], INIT_SPEED + (_data->level * 0.2f)); 
                }
                ctx.beep(500, 15);
            }
        }

        // Brick collision
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                _checkBallBrick(_balls[i], _bricks[r][c], c, r, ctx);
            }
        }

        // Check if ball fell out
        if (_balls[i].y > Console::H) {
            _balls[i].active = false;
            _combo = 1;
            if (_camera) _camera->shake(6);
            ctx.beep(200, 15);
        }
    }

    // Check life lost
    if (!anyBallActive) {
        ctx.sfxDeath();
        _data->lives--;
        _stickyPaddle = false;
        if (_data->lives <= 0) {
            // GameOver handled in Game update or we can directly push Game Over scene
        } else {
            _resetBall(true);
        }
    }

    // Level Clear
    if (_bricksLeft <= 0 && !_levelClearPause) {
        _levelClearPause = true;
        _clearTimer = 60;
        snprintf(_msg, sizeof(_msg), "LEVEL CLEAR!");
        _msgTimer = 60;
    }

    // Powerups
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (_powerUps[i].active) {
            _powerUps[i].y += 1.0f;
            if (rectIntersect(_powerUps[i].x - 4, _powerUps[i].y - 4, 8, 8, _padX, PAD_Y, _padW, 4)) {
                _applyPowerUp(_powerUps[i].type, ctx);
                _powerUps[i].active = false;
            } else if (_powerUps[i].y > Console::H) {
                _powerUps[i].active = false;
            }
        }
    }
}

void BBPlayScene::draw(Console& ctx) {
    drawField(ctx);
    
    // Draw Paddle
    ctx.drawBox((int)_padX, (int)PAD_Y, (int)_padW, 4);

    // Draw Balls
    for (int i = 0; i < MAX_BALLS; ++i) {
        if (_balls[i].active) {
            if (_balls[i].fireball) {
                if ((millis() / 40) % 2 == 0) {
                    ctx.drawBox((int)_balls[i].x - 1, (int)_balls[i].y - 1, 4, 4);
                } else {
                    ctx.drawBox((int)_balls[i].x, (int)_balls[i].y - 1, 2, 4);
                    ctx.drawBox((int)_balls[i].x - 1, (int)_balls[i].y, 4, 2);
                }
            } else {
                ctx.drawBox((int)_balls[i].x, (int)_balls[i].y, 2, 2);
            }
        }
    }

    // Draw Powerups
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (_powerUps[i].active) {
            const uint8_t* spr = nullptr;
            switch (_powerUps[i].type) {
                case PowerUpType::EXPAND: spr = spr_pw_expand; break;
                case PowerUpType::SHRINK: spr = spr_pw_shrink; break;
                case PowerUpType::CATCH: spr = spr_pw_catch; break;
                case PowerUpType::MULTIBALL: spr = spr_pw_multi; break;
                case PowerUpType::FIREBALL: spr = spr_pw_fire; break;
                case PowerUpType::LIFE: spr = spr_pw_life; break;
            }
            if (spr) {
                ctx.drawBitmapEx((int)_powerUps[i].x - 4, (int)_powerUps[i].y - 4, 1, 8, spr);
            }
        }
    }

    if (_particles) {
        _particles->draw(ctx);
    }

    // HUD
    ctx.beginScreenSpace();
    ctx.drawPrintf(0, 8, "Sc:%d", _data->score);
    ctx.drawPrintf(80, 8, "L:%d", _data->lives);
    
    if (_msgTimer > 0) {
        ctx.drawStrCentered(40, _msg);
    }
    ctx.endScreenSpace();
}

void BBPlayScene::drawField(Console& ctx) const {
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (_bricks[r][c].active) {
                int x = GRID_OX + c * BRICK_W;
                int y = GRID_OY + r * BRICK_H;
                if (_bricks[r][c].type == BrickType::HARD) {
                    ctx.drawBox(x + 1, y + 1, BRICK_W - 2, BRICK_H - 2);
                    ctx.drawFrame(x, y, BRICK_W, BRICK_H);
                } else if (_bricks[r][c].type == BrickType::SOLID) {
                    ctx.drawDitherBox(x, y, BRICK_W, BRICK_H, 2);
                } else {
                    ctx.drawFrame(x, y, BRICK_W, BRICK_H);
                }
            }
        }
    }
}

// ─── BBPauseScene ───────────────────────────────────────────────────────────
void BBPauseScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU2)) {
        sm.pop(ctx);
    }
    if (ctx.justPressed(Btn::MENU1)) {
        sm.clear(ctx); // Exit game
    }
}

void BBPauseScene::draw(Console& ctx) {
    _sm->drawUnder(ctx);
    Screens::drawPauseOverlay(ctx);
}

// ─── BBGameOverScene ────────────────────────────────────────────────────────
void BBGameOverScene::onEnter(Console& ctx) {
    _frame = 0;
}

void BBGameOverScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) {
        sm.clear(ctx);
        return;
    }
    if (_frame > 30) {
        if (ctx.justPressed(Btn::A)) {
            sm.emit(ctx, Event::CUSTOM_1); // Restart
        } else if (ctx.justPressed(Btn::B)) {
            sm.emit(ctx, Event::CUSTOM_2); // Title
        }
    }
}

void BBGameOverScene::draw(Console& ctx) {
    _sm->drawUnder(ctx); // Draw game behind
    Screens::drawGameOver(ctx, _data->score, _data->hiScore, _data->score >= _data->hiScore);
}

// ─── BrickBreakerGame ───────────────────────────────────────────────────────

void BrickBreakerGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();
    _play.setData(&_data);
    _play.setEngine(&_camera, &_particles);
    _gameover.setData(&_data);
    _gameover.setPlayScene(&_play);
    
    _sm.onEvent(Event::CUSTOM_1, SceneManager::REPLACE, &_play);
    _sm.onEvent(Event::CUSTOM_2, SceneManager::REPLACE, &_title);
    _sm.onEvent(Event::PAUSE, SceneManager::PUSH, &_pause);
    _sm.onEvent(Event::GAME_OVER, SceneManager::PUSH, &_gameover);
    
    _sm.replace(&_title, ctx);
}

void BrickBreakerGame::onExit(Console& ctx) {
    ctx.updateHiScore(_data.hiScore);
}

void BrickBreakerGame::update(Console& ctx, float dt) {
    if (_sm.current() == &_play) {
        if (_data.lives <= 0) {
            _sm.emit(ctx, Event::GAME_OVER);
        }
        _particles.update();
    }
    _sm.update(ctx, dt);
}

void BrickBreakerGame::draw(Console& ctx) {
    _sm.draw(ctx);
}

bool BrickBreakerGame::isRunning() const {
    return !_sm.empty();
}

const char* BrickBreakerGame::getName() const {
    return "BrickBreaker";
}

const uint8_t* BrickBreakerGame::getCoverArt() const {
    return nullptr; // Provide a cover art sprite if available in sprites file
}

// ────────────────────────────────────────────────────────────────────────────
// End of file

REGISTER_GAME(BrickBreakerGame);
