#include "SnakeGame.h"
#include <Preferences.h>

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void SnakeGame::onEnter(Console& ctx) {
    Preferences prefs;
    prefs.begin("snake", true);
    _hiScore = prefs.getUInt("hiscore", 0);
    String name = prefs.getString("hiname", "AAA");
    strncpy(_hiName, name.c_str(), 4);
    _hiName[3] = '\0'; // Ensure null termination
    prefs.end();

    _initRound();
    _running = true;
}

void SnakeGame::onExit(Console& ctx) {}

void SnakeGame::_initRound() {
    _state       = State::PLAYING;
    _dir         = Dir::RIGHT;
    _queueLen    = 0;
    _len         = 4;
    _score       = 0;
    _speed       = START_SPEED;
    _moveTimer   = 0;
    _newHiScore  = false;
    _bonusActive = false;
    _poisonActive = false;
    _numWalls    = 0;
    _shakeFrames = 0;

    for (int i = 0; i < _len; i++) {
        _sx[i] = (GRID_W / 2) - i;
        _sy[i] = GRID_H / 2;
    }

    _spawnApple();
}

// ─── Spawners ────────────────────────────────────────────────────────────────

bool SnakeGame::_isOccupied(int x, int y) const {
    for (int i = 0; i < _len; i++)      if (_sx[i] == x && _sy[i] == y) return true;
    for (int i = 0; i < _numWalls; i++) if (_wx[i] == x && _wy[i] == y) return true;
    if (x == _ax && y == _ay) return true;
    if (_bonusActive && x == _bx && y == _by) return true;
    if (_poisonActive && x == _px && y == _py) return true;
    return false;
}

void SnakeGame::_spawnApple() {
    int nx, ny;
    do { nx = random(GRID_W); ny = random(GRID_H); } while (_isOccupied(nx, ny));
    _ax = nx; _ay = ny;
}

void SnakeGame::_spawnBonus() {
    int nx, ny;
    do { nx = random(GRID_W); ny = random(GRID_H); } while (_isOccupied(nx, ny));
    _bx = nx; _by = ny;
    _bonusActive = true;
    _bonusTimer  = BONUS_DURATION;
}

void SnakeGame::_spawnPoison() {
    int nx, ny;
    do { nx = random(GRID_W); ny = random(GRID_H); } while (_isOccupied(nx, ny));
    _px = nx; _py = ny;
    _poisonActive = true;
    _poisonTimer  = POISON_DURATION;
}

void SnakeGame::_spawnWall() {
    if (_numWalls >= MAX_WALLS) return;
    int nx, ny;
    do { nx = random(GRID_W); ny = random(GRID_H); } while (_isOccupied(nx, ny));
    _wx[_numWalls] = nx;
    _wy[_numWalls] = ny;
    _numWalls++;
}

void SnakeGame::_pushInput(Dir d) {
    if (_queueLen < 2) _inputQueue[_queueLen++] = d;
}

void SnakeGame::_saveHighScore() {
    strncpy(_hiName, _currName, 4);
    _hiName[3] = '\0';
    
    Preferences prefs;
    prefs.begin("snake", false);
    prefs.putUInt("hiscore", _hiScore);
    prefs.putString("hiname", _hiName);
    prefs.end();
}

// ─── Update Logic ────────────────────────────────────────────────────────────

void SnakeGame::update(Console& ctx) {
    // Global Menu Return Handle
    if (ctx.justPressed(Btn::MENU1)) { 
        if (_state == State::NAME_ENTRY) _saveHighScore(); // Auto-save if they bail early
        _running = false; 
        return; 
    }

    if (_shakeFrames > 0) _shakeFrames--;

    // Pause Toggle
    if (ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU2)) {
        if (_state == State::PLAYING) _state = State::PAUSED;
        else if (_state == State::PAUSED) _state = State::PLAYING;
    }

    if (_state == State::PAUSED) return;

    switch (_state) {
        case State::PLAYING: {
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
                for (int i = 0; i < _len - 1; i++)      if (nx == _sx[i] && ny == _sy[i]) crashed = true;
                for (int i = 0; i < _numWalls; i++)     if (nx == _wx[i] && ny == _wy[i]) crashed = true;

                if (crashed) {
                    _shakeFrames = 15;
                    
                    if (_score > _hiScore) {
                        _newHiScore = true;
                        _hiScore = _score;
                        _nameIdx = 0;
                        _currName[0] = 'A'; _currName[1] = 'A'; _currName[2] = 'A'; _currName[3] = '\0';
                        _state = State::NAME_ENTRY;
                        ctx.beep(1200, 100); // Triumphant sound!
                    } else {
                        _state = State::DEAD;
                        ctx.sfxDeath();
                    }
                    break;
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
                    if (nx == _bx && ny == _by) {
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
                        _shakeFrames = 8;        
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
            break;
        }
        
        case State::NAME_ENTRY: {
            // Using repeat() makes it easy to scroll quickly by holding the button
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
                _saveHighScore();
                _state = State::DEAD;
                ctx.sfxMenuEnter();
            }
            break;
        }

        case State::DEAD: {
            if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) _initRound();
            break;
        }
    }
}

// ─── Drawing ─────────────────────────────────────────────────────────────────

void SnakeGame::draw(Console& ctx) {
    int ox = 0, oy = 0;
    if (_shakeFrames > 0) {
        ox = random(-1, 2);
        oy = random(-1, 2);
    }

    // Top UI bar
    ctx.drawHLine(0, TOP_OFFSET - 1, Console::W);
    ctx.setFont(u8g2_font_5x7_tf);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "SCR:%04u", (unsigned)_score);
    ctx.drawStr(2, 6, buf);
    
    if (!(_newHiScore && _state == State::PLAYING && (millis() / 200) % 2 == 0)) {
        snprintf(buf, sizeof(buf), "%s:%04u", _hiName, (unsigned)_hiScore);
        int w = ctx.strWidth(buf);
        ctx.drawStr(Console::W - w - 2, 6, buf);
    }

    // Draw Walls
    for (int i = 0; i < _numWalls; i++) {
        int px = (_wx[i] * BLOCK_SIZE) + ox;
        int py = (_wy[i] * BLOCK_SIZE) + TOP_OFFSET + oy;
        ctx.drawFrame(px, py, BLOCK_SIZE, BLOCK_SIZE);
        ctx.drawPixel(px + 1, py + 1);
        ctx.drawPixel(px + 2, py + 2);
    }

    // Draw Normal Apple
    ctx.drawDisc((_ax * BLOCK_SIZE) + (BLOCK_SIZE / 2) + ox, 
                 (_ay * BLOCK_SIZE) + (BLOCK_SIZE / 2) + TOP_OFFSET + oy, 1);
                 
    // Draw Bonus Apple
    if (_bonusActive && (millis() / 150) % 2 == 0) {
        ctx.drawDisc((_bx * BLOCK_SIZE) + (BLOCK_SIZE / 2) + ox, 
                     (_by * BLOCK_SIZE) + (BLOCK_SIZE / 2) + TOP_OFFSET + oy, 2);
    }

    // Draw Poison Apple
    if (_poisonActive) {
        int px = (_px * BLOCK_SIZE) + (BLOCK_SIZE / 2) + ox;
        int py = (_py * BLOCK_SIZE) + (BLOCK_SIZE / 2) + TOP_OFFSET + oy;
        ctx.drawLine(px - 1, py - 1, px + 1, py + 1);
        ctx.drawLine(px + 1, py - 1, px - 1, py + 1);
    }

    // Draw Snake Body
    for (int i = 0; i < _len; i++) {
        int px = (_sx[i] * BLOCK_SIZE) + ox;
        int py = (_sy[i] * BLOCK_SIZE) + TOP_OFFSET + oy;
        
        if (i == 0) {
            ctx.drawBox(px, py, BLOCK_SIZE, BLOCK_SIZE);
        } else {
            ctx.drawFrame(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
        }
    }

    // Overlays
    if (_state == State::NAME_ENTRY) {
        // Background Box
        ctx.setDrawColor(0);
        ctx.drawBox(14, 12, 100, 42);
        ctx.setDrawColor(1);
        ctx.drawFrame(14, 12, 100, 42);
        
        // Title
        ctx.setFont(u8g2_font_5x7_tf);
        int w = ctx.strWidth("NEW HIGH SCORE!");
        ctx.drawStr(64 - w/2, 22, "NEW HIGH SCORE!");
        
        // Interactive Letters
        ctx.setFont(u8g2_font_7x13B_tf);
        for(int i = 0; i < 3; i++) {
            char c[2] = {_currName[i], '\0'};
            int lx = 42 + (i * 14); // Spacing the letters out
            ctx.drawStr(lx, 38, c);
            
            // Blinking underline for the currently selected letter
            if (i == _nameIdx && (millis() / 250) % 2 == 0) {
                ctx.drawHLine(lx, 40, 7);
            }
        }
        
        // Footer prompt
        ctx.setFont(u8g2_font_5x7_tf);
        w = ctx.strWidth("Press A to save");
        ctx.drawStr(64 - w/2, 50, "Press A to save");
        
    } else if (_state == State::DEAD) {
        ctx.setDrawColor(0);
        ctx.drawBox(20, 20, 88, 28);
        ctx.setDrawColor(1);
        ctx.drawFrame(20, 20, 88, 28);
        ctx.setFont(u8g2_font_7x13B_tf);
        ctx.drawStr(28, 36, "GAME OVER");
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(28, 45, "Press A to restart");
        
    } else if (_state == State::PAUSED) {
        ctx.setDrawColor(0);
        ctx.drawBox(34, 24, 60, 18);
        ctx.setDrawColor(1);
        ctx.drawFrame(34, 24, 60, 18);
        ctx.setFont(u8g2_font_7x13B_tf);
        ctx.drawStr(44, 38, "PAUSED");
    }
}

// ─── Interface ────────────────────────────────────────────────────────────────
bool        SnakeGame::isRunning() const { return _running; }
const char* SnakeGame::getName()   const { return "Snake"; }
