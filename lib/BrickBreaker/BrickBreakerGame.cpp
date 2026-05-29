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
    if (ctx.justPressed(Btn::B)) {
        ctx.sfxMenuBack();
        sm.pop(ctx);
    }
    
    _bx += _bvx; _by += _bvy;
    if (_bx < 0 || _bx > Console::W) _bvx = -_bvx;
    if (_by < 0 || _by > Console::H) _bvy = -_bvy;
    
    _bx2 += _bvx2; _by2 += _bvy2;
    if (_bx2 < 0 || _bx2 > Console::W) _bvx2 = -_bvx2;
    if (_by2 < 0 || _by2 > Console::H) _bvy2 = -_bvy2;
}

void BBTitleScene::draw(Console& ctx) {
    ctx.drawBox(_bx, _by, 2, 2);
    ctx.drawBox(_bx2, _by2, 2, 2);
    
    Screens::drawTitle(ctx, "BRICK BREAKER");
    if (_data && _data->hiScore > 0) {
        ctx.setFont(u8g2_font_4x6_tr);
        ctx.drawPrintfCentered(45, "HI-SCORE: %lu", (unsigned long)_data->hiScore);
    }
}

// ─── BBPlayScene ────────────────────────────────────────────────────────────

void BBPlayScene::onEnter(Console& ctx) {
    _screenFlashFrames = 0;
    _levelFrames = 0;
    _combo = 1;
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
    _levelFrames = 0;
    _bricksLeft = 0;
    _stickyPaddle = false;

    for (int r = 0; r < ROWS; ++r) {
        _rowOffsetX[r] = 0;
        _rowSpeedX[r] = 0;
    }
    
    int level = _data->level;
    
    _boss.active = false;
    for (int i=0; i<3; i++) _projectiles[i].active = false;
    
    if (level > 0 && level % 5 == 0) {
        _boss.active = true;
        _boss.hp = _boss.maxHp = level * 5;
        _boss.x = Console::W / 2 - 12;
        _boss.y = 15;
        _boss.speedX = 0.5f + (level * 0.05f);
        _boss.attackTimer = 120;
        
        _bricksLeft = 1;
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                _bricks[r][c].active = false;
            }
        }
        return;
    }
    
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            bool active = false;
            BrickType type = BrickType::NORMAL;
            int hp = 1;

            if (level == 1) {
                // Basic block, top 3 rows
                if (r < 3) active = true;
            }
            else if (level == 2) {
                // Checkerboard
                if ((r + c) % 2 != 0 && r < 4) active = true;
            }
            else if (level == 3) {
                // Pyramid with HARD base
                int centerDist = abs(c - (COLS / 2));
                if (centerDist <= r && r < 4) active = true;
                if (r == 3 && active) { type = BrickType::HARD; hp = 2; }
            }
            else if (level == 4) {
                // Introduces EXPLOSIVE blocks
                if (r % 2 == 0) active = true;
                if (r == 0 && c % 3 == 1 && active) { type = BrickType::EXPLOSIVE; hp = 1; }
                if (r == 2 && c % 3 == 2 && active) { type = BrickType::HARD; hp = 2; }
            }
            else if (level == 5) {
                // Puzzle: Switch and Barrier
                if (r == 3) { active = true; type = BrickType::BARRIER; hp = 1; }
                else if (r == 0 && c == COLS / 2) { active = true; type = BrickType::SWITCH; hp = 1; }
                else if (r == 1 && c % 2 == 0) active = true;
            }
            else {
                // Level 6+: Procedural Mix
                active = (random(100) < 60); // 60% chance to have a brick
                if (r > 3) active = false; // Keep bottom clear
                
                if (active) {
                    int randType = random(100);
                    if (randType < 10) { type = BrickType::EXPLOSIVE; hp = 1; }
                    else if (randType < 30) { type = BrickType::HARD; hp = 2; }
                    else if (randType < 40 && r < 2) { type = BrickType::SOLID; hp = -1; }
                }
            }

            // Slowly increase overall difficulty by upgrading some random normal bricks to hard ones
            if (active && type == BrickType::NORMAL) {
                if (random(100) < _data->level * 2) {
                    type = BrickType::HARD;
                    hp = 2;
                }
            }

            _bricks[r][c].active = active;
            _bricks[r][c].type = type;
            _bricks[r][c].hp = hp;
            
            if (active && type != BrickType::SOLID && type != BrickType::BARRIER) {
                _bricksLeft++;
            }
        }
    }
    
    if (level > 5 && level % 5 != 0) {
        // Enforce Symmetry for procedurally generated levels
        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS / 2; ++c) {
                int mirrorC = COLS - 1 - c;
                if (_bricks[r][mirrorC].active && _bricks[r][mirrorC].type != BrickType::SOLID && _bricks[r][mirrorC].type != BrickType::BARRIER) {
                    _bricksLeft--;
                }
                
                _bricks[r][mirrorC] = _bricks[r][c];
                
                if (_bricks[r][mirrorC].active && _bricks[r][mirrorC].type != BrickType::SOLID && _bricks[r][mirrorC].type != BrickType::BARRIER) {
                    _bricksLeft++;
                }
            }
        }
    }

    // Set up slide-in animation and row movement for Level 6+
    for (int r = 0; r < ROWS; ++r) {
        _rowOffsetX[r] = (r % 2 == 0) ? -Console::W : Console::W;
        _rowSpeedX[r] = 0;
        
        if (level >= 6) {
            bool canMove = true;
            bool hasBricks = false;
            for (int c = 0; c < COLS; ++c) {
                if (_bricks[r][c].active) {
                    hasBricks = true;
                    if (_bricks[r][c].type == BrickType::SOLID || _bricks[r][c].type == BrickType::BARRIER) {
                        canMove = false;
                    }
                }
            }
            if (hasBricks && canMove && random(100) < 40) {
                _rowSpeedX[r] = (r % 2 == 0) ? 0.3f : -0.3f;
            }
        }
    }
    
    // Safety check: ensure at least one breakable brick exists
    if (_bricksLeft == 0) {
        _bricks[0][0].active = true;
        _bricks[0][0].type = BrickType::NORMAL;
        _bricks[0][0].hp = 1;
        _bricksLeft = 1;
    }

    _msgTimer = MESSAGE_DURATION;
    snprintf(_msg, sizeof(_msg), "Level %d", _data->level);
}

void BBPlayScene::_resetBall(bool serve) {
    for (int i = 1; i < MAX_BALLS; ++i) _balls[i].active = false;
    
    _padW = 20.0f; // Reset paddle size
    _padX = (Console::W - _padW) / 2.0f; // Center the paddle perfectly
    _combo = 1;
    
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
    
    _laserActive = false;
    _laserTimer = 0;
    for (int i = 0; i < MAX_LASERS; ++i) _lasers[i].active = false;
    
    // Clear any falling powerups from the previous life/level
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        _powerUps[i].active = false;
    }
}

void BBPlayScene::_normalizeBallVelocity(BBBall& b, float speedTarget) {
    float mag = sqrtf(b.vx * b.vx + b.vy * b.vy);
    if (mag > 0.001f) {
        b.vx = (b.vx / mag) * speedTarget;
        b.vy = (b.vy / mag) * speedTarget;
    }
}

void BBPlayScene::_spawnPowerUp(float x, float y) {
    if (random(100) > 15) return; // 15% chance for slightly better balance

    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (!_powerUps[i].active) {
            _powerUps[i].active = true;
            _powerUps[i].x = x;
            _powerUps[i].y = y;
            _powerUps[i].type = static_cast<PowerUpType>(random(7));
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
            _screenFlashFrames = 2;
            int activeBalls = 0;
            int sourceBallIdx = 0;
            for (int i = 0; i < MAX_BALLS; ++i) {
                if (_balls[i].active) {
                    activeBalls++;
                    sourceBallIdx = i;
                }
            }
            if (activeBalls > 0) {
                int spawnDir = 1;
                for (int i = 0; i < MAX_BALLS; ++i) {
                    if (!_balls[i].active) {
                        _balls[i] = _balls[sourceBallIdx];
                        _balls[i].sticky = false; // Prevent perfectly overlapping stuck balls
                        
                        // Scatter dynamically upwards and alternate directions
                        _balls[i].vx = spawnDir * (INIT_SPEED + random(0, 10) * 0.1f);
                        _balls[i].vy = -(INIT_SPEED + random(0, 10) * 0.1f);
                        spawnDir = -spawnDir; // Force next spawned ball to go the other way
                        
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
            _screenFlashFrames = 2;
            if (_data->lives < 5) _data->lives++;
            break;
        case PowerUpType::LASER:
            _screenFlashFrames = 2;
            _laserActive = true;
            _laserTimer = 400; // ~13 seconds
            break;
    }
}

void BBPlayScene::_destroyBrick(int c, int r, Console& ctx, bool isExplosion) {
    if (r < 0 || r >= ROWS || c < 0 || c >= COLS) return;
    if (!_bricks[r][c].active || _bricks[r][c].type == BrickType::SOLID) return;
    
    _bricks[r][c].active = false;
    if (_bricks[r][c].type != BrickType::BARRIER) {
        _bricksLeft--;
    }
    
    int baseScore = 10;
    if (_bricks[r][c].type == BrickType::HARD) baseScore = 20;
    else if (_bricks[r][c].type == BrickType::EXPLOSIVE) baseScore = 30;
    else if (_bricks[r][c].type == BrickType::SWITCH) baseScore = 50;
    
    _data->score += baseScore * _combo;
    ctx.updateHiScore(_data->score);
    
    _combo = min(20, _combo + 1);
    
    float brX = GRID_OX + c * BRICK_W + _rowOffsetX[r];
    float brY = GRID_OY + r * BRICK_H;
    
    if (!isExplosion) {
        _spawnPowerUp(brX + BRICK_W / 2.0f, brY + BRICK_H / 2.0f);
    }
    
    if (_particles) {
        for(int p=0; p<6; p++) {
            _particles->spawnPixel(brX + BRICK_W / 2.0f, brY + BRICK_H / 2.0f, random(-20, 20)*0.1f, random(-20, 20)*0.1f, random(10, 20));
        }
    }
    
    if (_bricks[r][c].type == BrickType::SWITCH) {
        ctx.beep(1500, 30);
        _screenFlashFrames = 2;
        if (_camera) _camera->shake(3);
        for (int rr = 0; rr < ROWS; ++rr) {
            for (int cc = 0; cc < COLS; ++cc) {
                if (_bricks[rr][cc].active && _bricks[rr][cc].type == BrickType::BARRIER) {
                    _destroyBrick(cc, rr, ctx, true);
                }
            }
        }
    }
    
    if (_bricks[r][c].type == BrickType::EXPLOSIVE) {
        ctx.beep(100, 40);
        if (_camera) _camera->shake(4);
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                if (dr == 0 && dc == 0) continue;
                _destroyBrick(c + dc, r + dr, ctx, true);
            }
        }
    }
}

bool BBPlayScene::_checkBallBrick(BBBall& ball, BBBrick& brick, int bx, int by, Console& ctx) {
    if (!brick.active) return false;

    float brX = GRID_OX + bx * BRICK_W + _rowOffsetX[by];
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
                ball.vx = (cX < bcX) ? -abs(ball.vx) : abs(ball.vx);
            } else {
                ball.vy = (cY < bcY) ? -abs(ball.vy) : abs(ball.vy);
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
                _destroyBrick(bx, by, ctx, false);
            }
        }
        int pitch = min(1000, 400 + (_combo * 40));
        ctx.beep(pitch, 10);
        if (_camera) _camera->shake(1);
    }
    return false;
}

void BBPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::B)) {
        ctx.sfxMenuBack();
        sm.push(new BBPauseScene(), ctx);
        return;
    }

    if (_screenFlashFrames > 0) _screenFlashFrames--;
    
    if (!_levelClearPause) {
        _levelFrames++;
    }

    if (_msgTimer > 0) _msgTimer--;
    if (_levelClearPause) {
        if (_clearTimer > 0) {
            _clearTimer--;
            // Pop random fireworks!
            if (_clearTimer % 15 == 0 && _particles) {
                _particles->spawnPixel(random(10, Console::W - 10), random(10, Console::H / 2), random(-30, 30)*0.1f, random(-30, 30)*0.1f, random(20, 50));
                ctx.beep(random(800, 1500), 5);
            }
        } else {
            _data->level++;
            _levelClearPause = false;
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

    // Update Moving Rows
    for (int r = 0; r < ROWS; ++r) {
        if (_levelFrames < 60) {
            _rowOffsetX[r] *= 0.85f; // Slide in animation
        } else if (_rowSpeedX[r] != 0) {
            _rowOffsetX[r] += _rowSpeedX[r];
            
            // Calculate row bounds dynamically
            float minX = 999;
            float maxX = -999;
            for (int c = 0; c < COLS; ++c) {
                if (_bricks[r][c].active) {
                    float bx = GRID_OX + c * BRICK_W + _rowOffsetX[r];
                    if (bx < minX) minX = bx;
                    if (bx + BRICK_W > maxX) maxX = bx + BRICK_W;
                }
            }
            
            // Bounce if hitting screen edges
            if (minX < 0 || maxX > Console::W) {
                _rowSpeedX[r] = -_rowSpeedX[r];
                _rowOffsetX[r] += _rowSpeedX[r] * 2;
            }
        }
    }
    
    // Update Boss
    if (_boss.active) {
        _boss.x += _boss.speedX;
        if (_boss.x <= 0 || _boss.x + _boss.w >= Console::W) {
            _boss.speedX = -_boss.speedX;
        }
        
        _boss.attackTimer--;
        if (_boss.attackTimer <= 0) {
            _boss.attackTimer = max(30, 120 - (_data->level * 5));
            for (int i=0; i<3; i++) {
                if (!_projectiles[i].active) {
                    _projectiles[i].active = true;
                    _projectiles[i].x = _boss.x + _boss.w/2;
                    _projectiles[i].y = _boss.y + _boss.h;
                    ctx.beep(800, 10);
                    break;
                }
            }
        }
    }
    
    // Update Projectiles
    for (int i=0; i<3; i++) {
        if (_projectiles[i].active) {
            _projectiles[i].y += 1.5f;
            if (_projectiles[i].y > Console::H) {
                _projectiles[i].active = false;
            } else if (rectIntersect(_projectiles[i].x - 1, _projectiles[i].y - 1, 3, 3, _padX, PAD_Y, _padW, 3)) {
                _projectiles[i].active = false;
                ctx.sfxDeath();
                _data->lives--;
                _stickyPaddle = false;
                if (_camera) _camera->shake(5);
                _screenFlashFrames = 5;
                if (_data->lives > 0) _resetBall(true);
                for (int j=0; j<3; j++) _projectiles[j].active = false;
            }
        }
    }

    // Launch Ball & Fire Lasers
    if (ctx.justPressed(Btn::A)) {
        for (int i = 0; i < MAX_BALLS; ++i) {
            if (_balls[i].active && _balls[i].sticky) {
                _msgTimer = 0; // Clear any active level overlay instantly
                _balls[i].sticky = false;
                _balls[i].vy = -INIT_SPEED;
                _balls[i].vx = ((_balls[i].x - (_padX + _padW/2.0f)) / (_padW/2.0f)) * INIT_SPEED;
                _normalizeBallVelocity(_balls[i], INIT_SPEED);
            }
        }
        
        // Fire Lasers
        if (_laserActive) {
            int spawned = 0;
            for (int i = 0; i < MAX_LASERS && spawned < 2; ++i) {
                if (!_lasers[i].active) {
                    _lasers[i].active = true;
                    _lasers[i].x = _padX + (spawned == 0 ? 0 : _padW - 1);
                    _lasers[i].y = PAD_Y - 4;
                    spawned++;
                }
            }
            if (spawned > 0) ctx.beep(1200, 5);
        }
    }
    
    // Update Lasers
    if (_laserActive) {
        if (_laserTimer > 0) _laserTimer--;
        else _laserActive = false;
    }
    
    for (int i = 0; i < MAX_LASERS; ++i) {
        if (!_lasers[i].active) continue;
        _lasers[i].y -= 4.0f; // Fast upward movement
        
        if (_lasers[i].y < 8) {
            _lasers[i].active = false;
            if (_particles) _particles->spawnPixel(_lasers[i].x, 8, 0, 1.0f, 10);
            continue;
        }
        bool hit = false;
        
        if (_boss.active && rectIntersect(_lasers[i].x, _lasers[i].y, 1, 4, _boss.x, _boss.y, _boss.w, _boss.h)) {
            hit = true;
            _lasers[i].active = false;
            _boss.hp--;
            ctx.beep(400, 10);
            if (_camera) _camera->shake(1);
            if (_particles) _particles->spawnPixel(_lasers[i].x, _lasers[i].y, random(-10, 10)*0.1f, random(0, 10)*0.1f, 15);
            
            if (_boss.hp <= 0) {
                _boss.active = false;
                _bricksLeft--;
                _data->score += 1000 * (_data->level / 5);
                ctx.updateHiScore(_data->score);
                ctx.beep(100, 50);
                if (_camera) _camera->shake(6);
                for(int p=0; p<40; p++) {
                    if(_particles) _particles->spawnPixel(_boss.x + _boss.w/2, _boss.y + _boss.h/2, random(-30, 30)*0.1f, random(-30, 30)*0.1f, random(20, 50));
                }
            }
        }
        
        for (int r = 0; r < ROWS && !hit; ++r) {
            for (int c = 0; c < COLS && !hit; ++c) {
                if (_bricks[r][c].active) {
                    float brX = GRID_OX + c * BRICK_W + _rowOffsetX[r];
                    float brY = GRID_OY + r * BRICK_H;
                    if (_lasers[i].x >= brX && _lasers[i].x <= brX + BRICK_W &&
                        _lasers[i].y >= brY && _lasers[i].y <= brY + BRICK_H) {
                        
                        hit = true;
                        _lasers[i].active = false;
                        
                        if (_bricks[r][c].type != BrickType::SOLID) {
                            _bricks[r][c].hp--;
                            
                            if (_bricks[r][c].hp <= 0) {
                                _destroyBrick(c, r, ctx, false);
                            } else if (_particles) {
                                _particles->spawnPixel(_lasers[i].x, _lasers[i].y, random(-10, 10)*0.1f, random(0, 10)*0.1f, 15);
                                ctx.beep(400, 10);
                            }
                        } else {
                            if (_particles) _particles->spawnPixel(_lasers[i].x, _lasers[i].y, random(-10, 10)*0.1f, random(0, 10)*0.1f, 15);
                            ctx.beep(300, 10);
                        }
                    }
                }
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

        // Ball trail
        if (_particles) {
            if (_balls[i].fireball) {
                if (random(100) < 60) {
                    _particles->spawnPixel(_balls[i].x, _balls[i].y, random(-10, 10)*0.02f, random(-10, 10)*0.02f, random(10, 20));
                }
            } else {
                if (random(100) < 40) { // Subtle ghost trail for normal balls
                    _particles->spawnPixel(_balls[i].x, _balls[i].y, 0, 0, random(3, 8));
                }
            }
        }

        // Wall collisions
        if (_balls[i].x <= 0) { _balls[i].x = 0; _balls[i].vx = -_balls[i].vx; ctx.beep(300, 10); }
        if (_balls[i].x >= Console::W - 1) { _balls[i].x = Console::W - 1; _balls[i].vx = -_balls[i].vx; ctx.beep(300, 10); }
        if (_balls[i].y <= 8) { _balls[i].y = 8; _balls[i].vy = -_balls[i].vy; ctx.beep(300, 10); }

        // AABB paddle collision
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
                    
                    // Impart "English" spin if moving the paddle while hitting
                    if (ctx.pressed(Btn::LEFT)) _balls[i].vx -= 0.6f;
                    if (ctx.pressed(Btn::RIGHT)) _balls[i].vx += 0.6f;
                    
                    _balls[i].vy = -INIT_SPEED;
                    _normalizeBallVelocity(_balls[i], min((float)MAX_SPEED, INIT_SPEED + (_data->level * 0.2f))); 
                }
                ctx.beep(500, 15);
            }
        }

        // Boss collision
        if (_boss.active && rectIntersect(_balls[i].x - 1, _balls[i].y - 1, 2, 2, _boss.x, _boss.y, _boss.w, _boss.h)) {
            float cX = _balls[i].x;
            float cY = _balls[i].y;
            float bcX = _boss.x + _boss.w / 2.0f;
            float bcY = _boss.y + _boss.h / 2.0f;
            
            if (!_balls[i].fireball) {
                if (abs(cX - bcX) / _boss.w > abs(cY - bcY) / _boss.h) {
                    _balls[i].vx = (cX < bcX) ? -abs(_balls[i].vx) : abs(_balls[i].vx);
                } else {
                    _balls[i].vy = (cY < bcY) ? -abs(_balls[i].vy) : abs(_balls[i].vy);
                }
            }
            
            _boss.hp -= (_balls[i].fireball) ? 2 : 1;
            ctx.beep(500, 10);
            if (_camera) _camera->shake(2);
            if (_particles) _particles->spawnPixel(_balls[i].x, _balls[i].y, 0, 0, 15);
            
            if (_boss.hp <= 0) {
                _boss.active = false;
                _bricksLeft--;
                _data->score += 1000 * (_data->level / 5);
                ctx.updateHiScore(_data->score);
                ctx.beep(100, 50);
                if (_camera) _camera->shake(6);
                for(int p=0; p<40; p++) {
                    if(_particles) _particles->spawnPixel(_boss.x + _boss.w/2, _boss.y + _boss.h/2, random(-30, 30)*0.1f, random(-30, 30)*0.1f, random(20, 50));
                }
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
        _clearTimer = 150; // Give time to read
        _screenFlashFrames = 3;
        _msgTimer = 0; // Disable normal msg so we can draw our own
        
        int parFrames = 60 * 30; // 30 seconds par time
        _lastTimeBonus = max(0, parFrames - _levelFrames) / 60 * 10;
        _lastLevelBonus = 100 * _data->level;
        int bonus = _lastLevelBonus + _lastTimeBonus;
        _data->score += bonus;
        ctx.updateHiScore(_data->score);
        
        ctx.beep(600, 10);
        ctx.beep(800, 10);
        ctx.beep(1000, 20);
    }

    // Powerups
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (_powerUps[i].active) {
            _powerUps[i].y += 1.0f;
            float hoverX = _powerUps[i].x + sin(_powerUps[i].y * 0.1f) * 3.0f;
            if (rectIntersect(hoverX - 4, _powerUps[i].y - 4, 8, 8, _padX, PAD_Y, _padW, 3)) {
                _applyPowerUp(_powerUps[i].type, ctx);
                _powerUps[i].active = false;
                if (_padX + _padW > Console::W) _padX = Console::W - _padW;
            } else if (_powerUps[i].y > Console::H) {
                _powerUps[i].active = false;
            }
        }
    }
}

void BBPlayScene::draw(Console& ctx) {
    drawField(ctx);
    
    if (_boss.active) {
        ctx.drawBox(_boss.x, _boss.y, _boss.w, _boss.h);
        ctx.setDrawColor(Console::COLOR_BLACK);
        
        // draw teeth pattern
        for (int i = 0; i < _boss.w; i += 4) {
            ctx.drawBox(_boss.x + i, _boss.y + _boss.h - 2, 2, 2);
        }
        
        ctx.drawBox(_boss.x + 4, _boss.y + 4, 4, 4); // left eye
        ctx.drawBox(_boss.x + _boss.w - 8, _boss.y + 4, 4, 4); // right eye
        ctx.setDrawColor(Console::COLOR_WHITE);
        
        // hp bar
        int hpBarW = (_boss.hp * _boss.w) / _boss.maxHp;
        ctx.drawBox(_boss.x, _boss.y - 3, hpBarW, 2);
    }
    
    for (int i=0; i<3; i++) {
        if (_projectiles[i].active) {
            ctx.drawBox(_projectiles[i].x - 1, _projectiles[i].y - 1, 3, 3);
        }
    }
    
    // Draw Paddle (Rounded pill shape, 3 pixels high)
    ctx.drawBox((int)_padX + 1, (int)PAD_Y, (int)_padW - 2, 3);
    ctx.drawBox((int)_padX, (int)PAD_Y + 1, 1, 1);
    ctx.drawBox((int)_padX + (int)_padW - 1, (int)PAD_Y + 1, 1, 1);
    
    if (_laserActive) {
        // Draw twin laser cannons on the paddle
        ctx.drawBox((int)_padX, (int)PAD_Y - 2, 2, 2);
        ctx.drawBox((int)_padX + (int)_padW - 2, (int)PAD_Y - 2, 2, 2);
    }
    
    // Draw Lasers
    for (int i = 0; i < MAX_LASERS; ++i) {
        if (_lasers[i].active) {
            ctx.drawLine((int)_lasers[i].x, (int)_lasers[i].y, (int)_lasers[i].x, (int)_lasers[i].y + 2);
        }
    }

    // Draw Balls
    for (int i = 0; i < MAX_BALLS; ++i) {
        if (_balls[i].active) {
            ctx.pushDrawState();
            ctx.setDrawColor(Console::COLOR_BLACK);
            
            // Draw black outline
            if (_balls[i].fireball) {
                if ((millis() / 40) % 2 == 0) {
                    ctx.drawBox((int)_balls[i].x - 2, (int)_balls[i].y - 2, 6, 6);
                } else {
                    ctx.drawBox((int)_balls[i].x - 1, (int)_balls[i].y - 2, 4, 6);
                    ctx.drawBox((int)_balls[i].x - 2, (int)_balls[i].y - 1, 6, 4);
                }
            } else {
                ctx.drawBox((int)_balls[i].x - 2, (int)_balls[i].y - 2, 4, 4);
            }
            
            // Draw white interior
            ctx.setDrawColor(Console::COLOR_WHITE);
            if (_balls[i].fireball) {
                if ((millis() / 40) % 2 == 0) {
                    ctx.drawBox((int)_balls[i].x - 1, (int)_balls[i].y - 1, 4, 4);
                } else {
                    ctx.drawBox((int)_balls[i].x, (int)_balls[i].y - 1, 2, 4);
                    ctx.drawBox((int)_balls[i].x - 1, (int)_balls[i].y, 4, 2);
                }
            } else {
                ctx.drawBox((int)_balls[i].x - 1, (int)_balls[i].y - 1, 2, 2);
            }
            
            ctx.popDrawState();
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
                case PowerUpType::LASER: spr = spr_pw_laser; break;
            }
            if (spr) {
                int hoverX = (int)(_powerUps[i].x + sin(_powerUps[i].y * 0.1f) * 3.0f);
                ctx.drawBitmapEx(hoverX - 4, (int)_powerUps[i].y - 4, 1, 8, spr);
            }
        }
    }

    if (_particles) {
        _particles->draw(ctx);
    }

    // HUD
    ctx.beginScreenSpace();
    
    // Top Bar Background
    ctx.pushDrawState();
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawBox(0, 0, Console::W, 7);
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.drawLine(0, 7, Console::W, 7);
    
    ctx.setFont(u8g2_font_4x6_tr);
    ctx.drawPrintf(2, 6, "%06d", _data->score);
    ctx.drawPrintfCentered(6, "LVL %d", _data->level);
    
    // Hearts for lives
    auto drawHeart = [](Console& c, int hx, int hy) {
        c.drawPixel(hx+1, hy); c.drawPixel(hx+3, hy);
        c.drawBox(hx, hy+1, 5, 2);
        c.drawBox(hx+1, hy+3, 3, 1);
        c.drawPixel(hx+2, hy+4);
    };
    
    for (int l = 0; l < _data->lives; ++l) {
        drawHeart(ctx, Console::W - 8 - (l * 7), 1);
    }
    
    // Dynamic Combo Multiplier
    if (_combo > 1) {
        if ((millis() / 150) % 2 == 0) {
            ctx.drawPrintf(Console::W - 16, Console::H - 8, "x%d", _combo);
        }
    }
    
    // Overlay Message
    if (_msgTimer > 0) {
        int boxW = 80;
        int boxH = 15;
        int boxX = (Console::W - boxW) / 2;
        int boxY = (Console::H - boxH) / 2;
        
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(boxX, boxY, boxW, boxH);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(boxX, boxY, boxW, boxH);
        
        ctx.drawStrCentered(boxY + 9, _msg);
    }
    
    if (_levelClearPause) {
        int boxW = 100;
        int boxH = 34;
        int boxX = (Console::W - boxW) / 2;
        int boxY = (Console::H - boxH) / 2;
        
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(boxX, boxY, boxW, boxH);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(boxX, boxY, boxW, boxH);
        
        ctx.drawStrCentered(boxY + 9, "LEVEL CLEAR!");
        
        char tb[32];
        snprintf(tb, sizeof(tb), "TIME BNS: %d", _lastTimeBonus);
        ctx.drawStrCentered(boxY + 19, tb);
        
        char lb[32];
        snprintf(lb, sizeof(lb), "LEVEL BNS: %d", _lastLevelBonus);
        ctx.drawStrCentered(boxY + 29, lb);
    }
    ctx.popDrawState();
    ctx.endScreenSpace();
    
    if (_screenFlashFrames > 0) {
        ctx.pushDrawState();
        ctx.setDrawColor(Console::COLOR_XOR);
        ctx.drawBox(0, 0, Console::W, Console::H);
        ctx.popDrawState();
    }
}

void BBPlayScene::drawField(Console& ctx) const {
    for (int r = 0; r < ROWS; ++r) {
        for (int c = 0; c < COLS; ++c) {
            if (_bricks[r][c].active) {
                int x = GRID_OX + c * BRICK_W + (int)_rowOffsetX[r];
                int y = GRID_OY + r * BRICK_H;
                if (_bricks[r][c].type == BrickType::HARD) {
                    ctx.drawBox(x + 1, y + 1, BRICK_W - 2, BRICK_H - 2);
                    ctx.drawFrame(x, y, BRICK_W, BRICK_H);
                    
                    if (_bricks[r][c].hp == 1) {
                        ctx.setDrawColor(Console::COLOR_BLACK);
                        // Draw a jagged crack
                        ctx.drawLine(x + 2, y + 1, x + 4, y + 3);
                        ctx.drawLine(x + 4, y + 3, x + 3, y + 5);
                        ctx.setDrawColor(Console::COLOR_WHITE);
                    }
                } else if (_bricks[r][c].type == BrickType::SOLID) {
                    ctx.drawDitherBox(x, y, BRICK_W, BRICK_H, 2);
                } else if (_bricks[r][c].type == BrickType::EXPLOSIVE) {
                    ctx.drawDitherBox(x, y, BRICK_W, BRICK_H, 3);
                    ctx.drawFrame(x, y, BRICK_W, BRICK_H);
                } else if (_bricks[r][c].type == BrickType::SWITCH) {
                    ctx.drawBox(x + 2, y + 1, BRICK_W - 4, BRICK_H - 2);
                } else if (_bricks[r][c].type == BrickType::BARRIER) {
                    ctx.drawDitherBox(x, y, BRICK_W, BRICK_H, 1);
                    ctx.drawFrame(x, y, BRICK_W, BRICK_H);
                } else {
                    ctx.drawFrame(x, y, BRICK_W, BRICK_H);
                }
            }
        }
    }
}

// ─── BBPauseScene ───────────────────────────────────────────────────────────
void BBPauseScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::B)) {
        sm.pop(ctx);
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

    if (_frame > 30) {
        if (ctx.justPressed(Btn::A)) {
            // Cleanly restart without stacking another play scene
            if (_play) _play->onEnter(ctx);
            sm.pop(ctx);
        } else if (ctx.justPressed(Btn::B)) {
            sm.clear(ctx);
            sm.emit(ctx, Event::CUSTOM_2); // Title
        }
    }
}

void BBGameOverScene::draw(Console& ctx) {
    _sm->drawUnder(ctx); // Draw game behind
    
    // Dim the background
    ctx.pushDrawState();
    ctx.drawDitherBox(0, 0, Console::W, Console::H, 2);
    
    // Draw a neat box
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawBox(10, 10, Console::W - 20, Console::H - 20);
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.drawFrame(10, 10, Console::W - 20, Console::H - 20);
    
    ctx.setFont(u8g2_font_6x10_tf);
    ctx.drawStrCentered(22, "GAME OVER");
    
    ctx.setFont(u8g2_font_4x6_tr);
    ctx.drawPrintfCentered(32, "SCORE: %d", _data->score);
    ctx.drawPrintfCentered(40, "HI-SCORE: %d", _data->hiScore);
    ctx.drawPrintfCentered(48, "LEVEL: %d", _data->level);
    
    if ((_frame / 30) % 2 == 0) {
        ctx.drawStrCentered(58, "A:RETRY  B:MENU");
    }
    ctx.popDrawState();
}

// ─── BrickBreakerGame ───────────────────────────────────────────────────────

void BrickBreakerGame::onEnter(Console& ctx) { ctx.setCPUSpeed(80);
    _data.hiScore = ctx.loadHiScore();
    _title.setData(&_data);
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
