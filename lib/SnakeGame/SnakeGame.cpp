#include "SnakeGame.h"
#include "GameRegistry.h"
#include "SnakeSprites.h"
#include "CommonScreens.h"

// ═════════════════════════════════════════════════════════════════════════════
// SnakePlayScene
// ═════════════════════════════════════════════════════════════════════════════

void SnakePlayScene::onEnter(Console& ctx) {
    _initRound();
}

void SnakePlayScene::_initRound() {
    _dir         = Dir::RIGHT;
    _queueLen    = 0;
    _len         = 4;
    _score       = 0;
    _speed       = START_SPEED;
    _moveTimer   = 0;
    _bonusActive = false;
    _poisonActive = false;
    _numWalls    = 0;
    _data->newHiScore = false;

    for (int i = 0; i < _len; i++) {
        _sx[i] = (GRID_W / 2) - i;
        _sy[i] = GRID_H / 2;
    }

    _spawnApple();
}

bool SnakePlayScene::_isOccupied(int x, int y) const {
    // 1. Check existing items
    for (int i = 0; i < _len; i++) {
        if (_sx[i] == x && _sy[i] == y) return true;
    }
    
    // Expand the "occupied" footprint of the walls to match their new 8x8 hitbox
    for (int i = 0; i < _numWalls; i++) {
        if (abs(x - _wx[i]) <= 1 && abs(y - _wy[i]) <= 1) return true;
    }
    
    if (x == _ax && y == _ay) return true;
    if (_bonusActive && abs(x - _bx) <= 1 && abs(y - _by) <= 1) return true;
    if (_poisonActive && x == _px && y == _py) return true;
    
    // 2. Protect the space exactly 1 step in front of the head
    int nextX = _sx[0];
    int nextY = _sy[0];
    
    // Check where the snake is currently heading
    Dir checkDir = (_queueLen > 0) ? _inputQueue[0] : _dir;
    if (checkDir == Dir::UP)    nextY--;
    if (checkDir == Dir::DOWN)  nextY++;
    if (checkDir == Dir::LEFT)  nextX--;
    if (checkDir == Dir::RIGHT) nextX++;
    
    // Apply screen wrap to the projected step
    if (nextX < 0) nextX = GRID_W - 1;
    else if (nextX >= GRID_W) nextX = 0;
    if (nextY < 0) nextY = GRID_H - 1;
    else if (nextY >= GRID_H) nextY = 0;
    
    if (x == nextX && y == nextY) return true;

    return false;
}

void SnakePlayScene::_spawnApple() {
    int nx, ny;
    do { nx = random(GRID_W); ny = random(GRID_H); } while (_isOccupied(nx, ny));
    _ax = nx; _ay = ny;
}

void SnakePlayScene::_spawnBonus() {
    int nx, ny;
    do { 
        // Restrict bounds so the 8x8 sprite doesn't clip the screen edges or UI
        nx = random(1, GRID_W - 1); 
        ny = random(1, GRID_H - 1); 
    } while (_isOccupied(nx, ny));
    _bx = nx; _by = ny;
    _bonusActive = true;
    _bonusTimer  = BONUS_DURATION;
}

void SnakePlayScene::_spawnPoison() {
    int nx, ny;
    do { nx = random(GRID_W); ny = random(GRID_H); } while (_isOccupied(nx, ny));
    _px = nx; _py = ny;
    _poisonActive = true;
    _poisonTimer  = POISON_DURATION;
}

void SnakePlayScene::_spawnWall() {
    if (_numWalls >= MAX_WALLS) return;
    int nx, ny;
    do { 
        // Restrict bounds so the 8x8 wall sprite doesn't clip the screen edges or UI
        nx = random(1, GRID_W - 1); 
        ny = random(1, GRID_H - 1); 
    } while (_isOccupied(nx, ny));
    _wx[_numWalls] = nx;
    _wy[_numWalls] = ny;
    _numWalls++;
}

void SnakePlayScene::_pushInput(Dir d) {
    if (_queueLen < 2) _inputQueue[_queueLen++] = d;
}

void SnakePlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    if (ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU2)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::PAUSE);
        return;
    }

    Dir checkDir = (_queueLen > 0) ? _inputQueue[_queueLen - 1] : _dir;

    if (ctx.justPressed(Btn::UP)    && checkDir != Dir::DOWN && checkDir != Dir::UP)    _pushInput(Dir::UP);
    if (ctx.justPressed(Btn::DOWN)  && checkDir != Dir::UP   && checkDir != Dir::DOWN)  _pushInput(Dir::DOWN);
    if (ctx.justPressed(Btn::LEFT)  && checkDir != Dir::RIGHT && checkDir != Dir::LEFT) _pushInput(Dir::LEFT);
    if (ctx.justPressed(Btn::RIGHT) && checkDir != Dir::LEFT  && checkDir != Dir::RIGHT) _pushInput(Dir::RIGHT);

    if (++_moveTimer >= _speed) {
        _moveTimer = 0;

        if (_queueLen > 0) {
            _dir = _inputQueue[0];
            _inputQueue[0] = _inputQueue[1];
            _queueLen--;
        }

        int nx = _sx[0];
        int ny = _sy[0];

        if (_dir == Dir::UP)    ny--;
        if (_dir == Dir::DOWN)  ny++;
        if (_dir == Dir::LEFT)  nx--;
        if (_dir == Dir::RIGHT) nx++;

        // 1. Screen Wrapping
        if (nx < 0) nx = GRID_W - 1;
        else if (nx >= GRID_W) nx = 0;
        
        if (ny < 0) ny = GRID_H - 1;
        else if (ny >= GRID_H) ny = 0;

        // 2. Wall & Self Collision
        bool crashed = false;
        
        // Self-collision remains grid-based (the body moves strictly on the grid)
        for (int i = 0; i < _len - 1; i++) {
            if (nx == _sx[i] && ny == _sy[i]) crashed = true;
        }

        // Wall collision upgraded to pixel-perfect AABB for 1-pixel forgiveness
        Rect snakeHead = {nx * BLOCK_SIZE, ny * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE};
        
        for (int i = 0; i < _numWalls; i++) {
            // The wall sprite is 8x8, offset by -2 from the grid block's top-left
            Rect wall = {
                (_wx[i] * BLOCK_SIZE) - 2, 
                (_wy[i] * BLOCK_SIZE) - 2, 
                8, 
                8
            };
            
            // Shave exactly 1 pixel off every side of the wall's hitbox
            wall = wall.inset(1, 1); 
            
            if (snakeHead.overlaps(wall)) {
                crashed = true;
            }
        }

        if (crashed) {
            if (_camera) _camera->shake(15);
            _data->lastScore = _score;
            
            if (_score > _data->hiScore) {
                _data->newHiScore = true;
                _data->hiScore = _score;
                ctx.beep(1200, 100); // Triumphant sound!
                sm.emit(ctx, Event::CUSTOM_2); // NameEntry
            } else {
                _data->newHiScore = false;
                ctx.sfxDeath();
                sm.emit(ctx, Event::GAME_OVER); // DeadScene
            }
            return;
        }

        // 3. Apple Collection
        if (nx == _ax && ny == _ay) {
            if (_len < MAX_SNAKE) _len++;
            _score += 10;
            
            if (_score % 100 == 0) {
                if (_speed > 2) _speed--;
                _spawnWall();
            }
            
            ctx.sfxPoint();
            _spawnApple();
            
            if (!_bonusActive && random(100) < 15) _spawnBonus();
            if (!_poisonActive && random(100) < 20) _spawnPoison();
        }
        
        // 4. Bonus Collection
        if (_bonusActive) {
            if (abs(nx - _bx) <= 1 && abs(ny - _by) <= 1) {
                // Trigger the massive bonus explosion exactly where the apple was
                _spawnSparks(_bx, _by, SparkType::BONUS); 
                
                _score += BONUS_POINTS;
                _bonusActive = false;
                ctx.beep(1500, 80); 
            } else if (--_bonusTimer == 0) {
                _bonusActive = false;
            }
        }

        // 5. Poison Collection
        if (_poisonActive) {
            if (nx == _px && ny == _py) {
                _score = (_score >= 20) ? _score - 20 : 0;
                if (_len > 4) _len -= 2; 
                if (_camera) _camera->shake(8);        
                _poisonActive = false;
                ctx.beep(200, 100);      
            } else if (--_poisonTimer == 0) {
                _poisonActive = false;
            }
        }

        // Shift Body
        for (int i = _len - 1; i > 0; i--) {
            _sx[i] = _sx[i - 1];
            _sy[i] = _sy[i - 1];
        }

        // Update Head
        _sx[0] = nx;
        _sy[0] = ny;
    }
}

void SnakePlayScene::_spawnSparks(int gridX, int gridY, SparkType type) {
    int px = (gridX * BLOCK_SIZE) + (BLOCK_SIZE / 2);
    int py = (gridY * BLOCK_SIZE) + TOP_OFFSET + (BLOCK_SIZE / 2);

    uint8_t targetSpawn = (type == SparkType::BONUS) ? 20 : 6;
    for (int i = 0; i < targetSpawn; i++) {
        if (type == SparkType::BONUS) {
            _particles->spawnPixel(px, py, (random(-30, 31) / 10.0f), (random(-30, 31) / 10.0f), random(15, 35));
        } else if (type == SparkType::NORMAL) {
            _particles->spawnPixel(px, py, (random(-10, 11) / 5.0f), (random(-15, -5) / 5.0f), random(10, 20));
        } else { // POISON
            _particles->spawnPixel(px, py, (random(-10, 11) / 5.0f), (random(5, 15) / 5.0f), random(10, 20));
        }
    }
}

void SnakePlayScene::drawField(Console& ctx) const {
    ctx.setCamera(nullptr); // Unset camera for UI

    // Top UI bar
    ctx.drawHLine(0, TOP_OFFSET - 1, Console::W);
    ctx.setFont(u8g2_font_5x7_tf);
    
    ctx.drawPrintf(2, 6, "SCR:%04u", (unsigned)_score);
    
    // Blink hi-score if a new record was just achieved
    if (!(_data->newHiScore && (millis() / 200) % 2 == 0)) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%s:%04u", _data->hiName, (unsigned)_data->hiScore);
        ctx.drawStrRight(Console::W - 2, 6, buf);
    }

    ctx.setCamera(_camera); // Apply camera shake to the game world

    // Draw Walls (8x8)
    // Offset by -2 pixels to center the 8x8 sprite over the 4x4 block
    for (int i = 0; i < _numWalls; i++) {
        int px = (_wx[i] * BLOCK_SIZE) - 2;
        int py = (_wy[i] * BLOCK_SIZE) + TOP_OFFSET - 2;
        ctx.drawBitmap(px, py, 1, 8, spr_snake_wall);
    }

    _particles->draw(ctx);

    // Draw Normal Apple (4x4)
    // No offset needed, fits perfectly in the 4x4 grid block
    int ax_px = (_ax * BLOCK_SIZE); 
    int ay_py = (_ay * BLOCK_SIZE) + TOP_OFFSET;
    ctx.drawBitmap(ax_px, ay_py, 1, 4, spr_snake_apple);
                 
    // Draw Bonus Apple (8x8)
    // Offset by -2 pixels to center the 8x8 sprite over the 4x4 block
    if (_bonusActive && (millis() / 150) % 2 == 0) {
        int bx_px = (_bx * BLOCK_SIZE) - 2;
        int by_py = (_by * BLOCK_SIZE) + TOP_OFFSET - 2;
        ctx.drawBitmap(bx_px, by_py, 1, 8, spr_snake_bonus);
    }

    // Draw Poison Apple (4x4)
    // No offset needed
    if (_poisonActive) {
        int px_px = (_px * BLOCK_SIZE);
        int py_py = (_py * BLOCK_SIZE) + TOP_OFFSET;
        ctx.drawBitmap(px_px, py_py, 1, 4, spr_snake_poison);
    }

    // Draw Snake Body
    for (int i = 0; i < _len; i++) {
        int px = (_sx[i] * BLOCK_SIZE);
        int py = (_sy[i] * BLOCK_SIZE) + TOP_OFFSET;
        
        // 1. Draw the segment itself
        if (i == 0) {
            ctx.drawBox(px, py, BLOCK_SIZE, BLOCK_SIZE); // Head is always solid
        } else {
            ctx.drawBox(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2); // Thinner body for a sleek look
        }

        // 2. Draw the connector to the PREVIOUS segment (the one closer to the head)
        if (i > 0) {
            int prevX = (_sx[i-1] * BLOCK_SIZE);
            int prevY = (_sy[i-1] * BLOCK_SIZE) + TOP_OFFSET;

            // Skip connection if wrapping around screen edges
            if (abs(_sx[i] - _sx[i-1]) <= 1 && abs(_sy[i] - _sy[i-1]) <= 1) {
                // Horizontal connection
                if (_sx[i] != _sx[i-1]) {
                    int connX = min(px, prevX) + 1;
                    ctx.drawBox(connX, py + 1, BLOCK_SIZE, BLOCK_SIZE - 2);
                }
                // Vertical connection
                else if (_sy[i] != _sy[i-1]) {
                    int connY = min(py, prevY) + 1;
                    ctx.drawBox(px + 1, connY, BLOCK_SIZE - 2, BLOCK_SIZE);
                }
            }
        }
    }
    
    ctx.setCamera(nullptr); // Unset camera to prevent UI from shaking in overlay scenes
}

void SnakePlayScene::draw(Console& ctx) {
    drawField(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
// SnakePauseScene
// ═════════════════════════════════════════════════════════════════════════════

void SnakePauseScene::onEnter(Console& ctx) {}

void SnakePauseScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }

    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
    }
}

void SnakePauseScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    Screens::drawPauseOverlay(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
// SnakeNameEntryScene
// ═════════════════════════════════════════════════════════════════════════════

void SnakeNameEntryScene::onEnter(Console& ctx) {
    _nameIdx = 0;
    _currName[0] = 'A'; _currName[1] = 'A'; _currName[2] = 'A'; _currName[3] = '\0';
    _frame = 0;
}

void SnakeNameEntryScene::_saveToNVS(Console& ctx) {
    strncpy(_data->hiName, _currName, 4);
    _data->hiName[3] = '\0';
    
    ctx.saveUInt("hiscore", _data->hiScore);
    ctx.saveStr("hiname", _data->hiName);
}

void SnakeNameEntryScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    
    if (ctx.justPressed(Btn::MENU1)) { 
        _saveToNVS(ctx); 
        sm.emit(ctx, Event::QUIT); 
        return; 
    }

    if (ctx.justPressed(Btn::UP) || ctx.repeat(Btn::UP)) {
        _currName[_nameIdx]++;
        if (_currName[_nameIdx] > 'Z') _currName[_nameIdx] = 'A';
        ctx.sfxMenuNav();
    }
    if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) {
        _currName[_nameIdx]--;
        if (_currName[_nameIdx] < 'A') _currName[_nameIdx] = 'Z';
        ctx.sfxMenuNav();
    }
    if (ctx.justPressed(Btn::LEFT) || ctx.repeat(Btn::LEFT)) {
        if (_nameIdx > 0) _nameIdx--;
        else _nameIdx = 2; // Wrap left
        ctx.sfxMenuNav();
    }
    if (ctx.justPressed(Btn::RIGHT) || ctx.repeat(Btn::RIGHT)) {
        if (_nameIdx < 2) _nameIdx++;
        else _nameIdx = 0; // Wrap right
        ctx.sfxMenuNav();
    }
    if (ctx.justPressed(Btn::A)) {
        _saveToNVS(ctx);
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::GAME_OVER); // DeadScene
    }
}

void SnakeNameEntryScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);

    ctx.setDrawColor(0);
    ctx.drawBox(14, 12, 100, 42);
    ctx.setDrawColor(1);
    ctx.drawFrame(14, 12, 100, 42);
    
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawStrCentered(22, "NEW HIGH SCORE!");
    
    ctx.setFont(u8g2_font_7x13B_tf);
    for(int i = 0; i < 3; i++) {
        char c[2] = {_currName[i], '\0'};
        int lx = 42 + (i * 14); 
        ctx.drawStr(lx, 38, c);
        
        if (i == _nameIdx && (millis() / 250) % 2 == 0) {
            ctx.drawHLine(lx, 40, 7);
        }
    }
    
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.drawStrCentered(50, "Press A to save");
}

// ═════════════════════════════════════════════════════════════════════════════
// SnakeDeadScene
// ═════════════════════════════════════════════════════════════════════════════

void SnakeDeadScene::onEnter(Console& ctx) {
    _frame = 0;
}

void SnakeDeadScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // PlayScene
    }
}

void SnakeDeadScene::draw(Console& ctx) {
    Screens::drawGameOver(ctx, _data->lastScore, _data->hiScore, false);
}

// ═════════════════════════════════════════════════════════════════════════════
// SnakeGame — Framework Integration
// ═════════════════════════════════════════════════════════════════════════════

void SnakeGame::onEnter(Console& ctx) { ctx.setCPUSpeed(80);
    _data.hiScore = ctx.loadUInt("hiscore", 0);
    ctx.loadStr("hiname", _data.hiName, sizeof(_data.hiName), "AAA");

    // Wire sibling pointers
    _play.setData(&_data);
    _play.setEngine(&_camera, &_particles);

    _nameEntry.setData(&_data);
    _nameEntry.setEngine(&_camera);

    _dead.setData(&_data);
    _dead.setEngine(&_camera);

    useDefaultEvents(&_pause, &_dead);
    _sm.onEvent(Event::CUSTOM_1,  SceneManager::REPLACE, &_play); // Start/Restart
    _sm.onEvent(Event::CUSTOM_2,  SceneManager::REPLACE, &_nameEntry); // Name Entry

    _sm.replace(&_play, ctx);
}

const char* SnakeGame::getName()     const { return "Snake"; }
const uint8_t* SnakeGame::getCoverArt() const { return spr_snake_cover; }

REGISTER_GAME(SnakeGame);
