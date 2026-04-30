#include "SnakeGame.h"

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void SnakeGame::onEnter(Console& ctx) {  // <--- Added Console& ctx
    _initRound();
    _running = true;
}

void SnakeGame::onExit(Console& ctx) {}  // <--- Added Console& ctx

void SnakeGame::_initRound() {
    _state     = State::PLAYING;
    _dir       = Dir::RIGHT;
    _nextDir   = Dir::RIGHT;
    _len       = 4;
    _score     = 0;
    _speed     = START_SPEED;
    _moveTimer = 0;

    // Start in the middle of the screen
    for (int i = 0; i < _len; i++) {
        _sx[i] = (GRID_W / 2) - i;
        _sy[i] = GRID_H / 2;
    }

    _spawnApple();
}

void SnakeGame::_spawnApple() {
    bool valid = false;
    while (!valid) {
        _ax = random(GRID_W);
        _ay = random(GRID_H);
        valid = true;
        
        // Make sure apple doesn't spawn inside the snake
        for (int i = 0; i < _len; i++) {
            if (_sx[i] == _ax && _sy[i] == _ay) {
                valid = false;
                break;
            }
        }
    }
}

// ─── Update Logic ────────────────────────────────────────────────────────────

void SnakeGame::update(Console& ctx) {
    if (ctx.justPressed(Btn::MENU1)) {
        _running = false;
        return;
    }

    switch (_state) {
        case State::PLAYING: {
            // Buffer the input so we don't accidentally reverse into ourselves
            // if two buttons are pressed in the same frame window.
            if (ctx.justPressed(Btn::UP)    && _dir != Dir::DOWN)  _nextDir = Dir::UP;
            if (ctx.justPressed(Btn::DOWN)  && _dir != Dir::UP)    _nextDir = Dir::DOWN;
            if (ctx.justPressed(Btn::LEFT)  && _dir != Dir::RIGHT) _nextDir = Dir::LEFT;
            if (ctx.justPressed(Btn::RIGHT) && _dir != Dir::LEFT)  _nextDir = Dir::RIGHT;

            // Only move every few frames based on current speed
            if (++_moveTimer >= _speed) {
                _moveTimer = 0;
                _dir = _nextDir;

                // Calculate new head position
                int nx = _sx[0];
                int ny = _sy[0];

                if (_dir == Dir::UP)    ny--;
                if (_dir == Dir::DOWN)  ny++;
                if (_dir == Dir::LEFT)  nx--;
                if (_dir == Dir::RIGHT) nx++;

                // 1. Check wall collisions
                if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) {
                    _state = State::DEAD;
                    ctx.sfxDeath();
                    break;
                }

                // 2. Check self collision (ignoring the tail tip since it moves)
                for (int i = 0; i < _len - 1; i++) {
                    if (nx == _sx[i] && ny == _sy[i]) {
                        _state = State::DEAD;
                        ctx.sfxDeath();
                        break;
                    }
                }

                if (_state == State::DEAD) break;

                // 3. Check apple collection
                bool ateApple = (nx == _ax && ny == _ay);
                
                if (ateApple) {
                    if (_len < MAX_SNAKE) _len++;
                    _score += 10;
                    if (_score > _hiScore) _hiScore = _score;
                    
                    // Increase speed slightly every 50 points
                    if (_score % 50 == 0 && _speed > 1) _speed--;
                    
                    ctx.sfxPoint();
                    _spawnApple();
                }

                // 4. Shift body segments down the array
                for (int i = _len - 1; i > 0; i--) {
                    _sx[i] = _sx[i - 1];
                    _sy[i] = _sy[i - 1];
                }

                // 5. Update head
                _sx[0] = nx;
                _sy[0] = ny;
            }
            break;
        }

        case State::DEAD: {
            if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
                _initRound();
            }
            break;
        }
    }
}

// ─── Drawing ─────────────────────────────────────────────────────────────────

void SnakeGame::draw(Console& ctx) {
    // Top UI bar
    ctx.drawHLine(0, TOP_OFFSET - 1, Console::W);
    ctx.setFont(u8g2_font_5x7_tf);
    
    char buf[16];
    snprintf(buf, sizeof(buf), "SCR:%04u", (unsigned)_score);
    ctx.drawStr(2, 6, buf);
    
    snprintf(buf, sizeof(buf), "HI:%04u", (unsigned)_hiScore);
    int w = ctx.strWidth(buf);
    ctx.drawStr(Console::W - w - 2, 6, buf);

    // Draw Apple (drawDisc uses center coords)
    ctx.drawDisc((_ax * BLOCK_SIZE) + (BLOCK_SIZE / 2), 
                 (_ay * BLOCK_SIZE) + (BLOCK_SIZE / 2) + TOP_OFFSET, 
                 1);

    // Draw Snake
    for (int i = 0; i < _len; i++) {
        int px = _sx[i] * BLOCK_SIZE;
        int py = (_sy[i] * BLOCK_SIZE) + TOP_OFFSET;
        
        if (i == 0) {
            // Solid box for the head
            ctx.drawBox(px, py, BLOCK_SIZE, BLOCK_SIZE);
        } else {
            // Hollow frame with inset for the body segments so you can see the joints
            ctx.drawFrame(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2);
        }
    }

    // Draw Game Over overlay
    if (_state == State::DEAD) {
        ctx.setDrawColor(0);
        ctx.drawBox(20, 20, 88, 28);
        ctx.setDrawColor(1);
        ctx.drawFrame(20, 20, 88, 28);
        
        ctx.setFont(u8g2_font_7x13B_tf);
        ctx.drawStr(28, 36, "GAME OVER");
        
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(28, 45, "Press A to restart");
    }
}

// ─── Interface ────────────────────────────────────────────────────────────────
bool        SnakeGame::isRunning() const { return _running; }
const char* SnakeGame::getName()   const { return "Snake"; }