// Target path: lib/PongGame/PongGame.cpp
#include "PongGame.h"

// ═════════════════════════════════════════════════════════════════════════════
// PongTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void PongTitleScene::onEnter(Console& ctx) {
    _frame = 0;
}

void PongTitleScene::update(Console& ctx, SceneManager& sm) {
    _frame++;

    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);
    }
}

void PongTitleScene::draw(Console& ctx) {
    // Title
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(48, 22, "PONG");

    // Divider
    ctx.drawHLine(0, 28, Console::W);

    // Mini court preview — two paddles and a centre line
    ctx.drawVLine(64, 32, 24);        // centre dashes (approximate)
    ctx.drawBox(8,  38, PongState::PAD_W, PongState::PAD_H);
    ctx.drawBox(Console::W - 8 - PongState::PAD_W, 38, PongState::PAD_W, PongState::PAD_H);

    // Blinking prompt
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(28, 58, "Press A to play");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// PongPlayScene
// ═════════════════════════════════════════════════════════════════════════════

void PongPlayScene::onEnter(Console& ctx) {
    _st.scoreL    = 0;
    _st.scoreR    = 0;
    _playerWon    = false;
    _st.leftY     = PongState::FIELD_TOP + (PongState::FIELD_H - PongState::PAD_H) / 2.0f;
    _st.rightY    = _st.leftY;
    _resetBall(true);
}

void PongPlayScene::_resetBall(bool serveLeft) {
    _st.ballPos = {
        Console::W / 2.0f,
        PongState::FIELD_TOP + PongState::FIELD_H / 2.0f
    };
    float vx = serveLeft ? PongState::BALL_SPEED : -PongState::BALL_SPEED;
    float vy = ((millis() & 1) ? 1.0f : -1.0f) * PongState::BALL_SPEED * 0.55f;
    _st.ballVel = { vx, vy };
    
    // Set 3-second delay (90 frames)
    _serveTimer = 90;
}

void PongPlayScene::_updateAI() {
    float padCY  = _st.rightY + PongState::PAD_H / 2.0f;
    float ballCY = _st.ballPos.y;
    float diff   = ballCY - padCY;
    // Clamp to AI speed so it can be beaten
    float move = gclamp(diff, -PongState::AI_SPEED, PongState::AI_SPEED);
    _st.rightY += move;
    float topBound = (float)PongState::FIELD_TOP;
    float botBound = (float)(PongState::FIELD_TOP + PongState::FIELD_H - PongState::PAD_H);
    _st.rightY = gclamp(_st.rightY, topBound, botBound);
}

void PongPlayScene::_spawnSparks(float x, float y, float dirX) {
    uint8_t spawned = 0;
    for (auto& p : _particles) {
        if (p.life == 0) {
            p.x = x;
            p.y = y;
            p.vx = dirX * (random(15, 35) / 10.0f); // Burst outward
            p.vy = (random(-25, 25) / 10.0f);       // Spread vertically
            p.life = random(8, 15);
            if (++spawned > 5) break;               // 5 sparks per hit
        }
    }
}

void PongPlayScene::_handlePaddleCollision(Console& ctx) {
    auto& s = _st;

    // Left paddle (player) - Hitbox expanded by BALL_R
    float lpx = (float)(PongState::PAD_MARGIN);
    if (s.ballVel.x < 0 &&
        s.ballPos.x - PongState::BALL_R <= lpx + PongState::PAD_W &&
        s.ballPos.x - PongState::BALL_R >= lpx - 2.0f &&
        s.ballPos.y + PongState::BALL_R >= s.leftY &&
        s.ballPos.y - PongState::BALL_R <= s.leftY + PongState::PAD_H) {

        s.ballPos.x = lpx + PongState::PAD_W + PongState::BALL_R;
        s.ballVel.x = -(s.ballVel.x) * 1.05f;  
        float rel  = (s.ballPos.y - s.leftY) / PongState::PAD_H - 0.5f;
        s.ballVel.y = rel * PongState::BALL_SPEED * 2.2f;
        
        _shakeFrames = 3;
        _leftHitTimer = 4;
        _spawnSparks(s.ballPos.x, s.ballPos.y, 1.0f);
        ctx.beep(600 + (_rallyCount * 40) + abs((int)s.ballVel.y) * 50, 30);
    }

    // Right paddle (AI) - Hitbox expanded by BALL_R
    float rpx = (float)(Console::W - PongState::PAD_MARGIN - PongState::PAD_W);
    if (s.ballVel.x > 0 &&
        s.ballPos.x + PongState::BALL_R >= rpx &&
        s.ballPos.x + PongState::BALL_R <= rpx + PongState::PAD_W + 2.0f &&
        s.ballPos.y + PongState::BALL_R >= s.rightY &&
        s.ballPos.y - PongState::BALL_R <= s.rightY + PongState::PAD_H) {

        s.ballPos.x = rpx - PongState::BALL_R;
        s.ballVel.x = -(s.ballVel.x) * 1.05f;
        float rel  = (s.ballPos.y - s.rightY) / PongState::PAD_H - 0.5f;
        s.ballVel.y = rel * PongState::BALL_SPEED * 2.2f;
        
        _shakeFrames = 3;
        _rightHitTimer = 4;
        _spawnSparks(s.ballPos.x, s.ballPos.y, -1.0f);
        ctx.beep(600 + abs((int)s.ballVel.y) * 50, 30); 
    }

    s.ballVel.x = gclamp(s.ballVel.x, -PongState::MAX_SPEED, PongState::MAX_SPEED);
    s.ballVel.y = gclamp(s.ballVel.y, -PongState::MAX_SPEED, PongState::MAX_SPEED);
}

// Helper used by both PlayScene and PauseScene (the latter draws background).
// Helper used by both PlayScene and PauseScene (the latter draws background).
void PongPlayScene::drawField(Console& ctx, int ox, int oy) const {
    const PongState& s = _st;
    
    // HUD line + scores
    ctx.setFont(u8g2_font_5x7_tf);
    
    // If the serve timer is high (just scored), jump the numbers up by 3 pixels
    int scoreOffsetY = (_serveTimer > 70) ? -3 : 0; 

    char buf[4];
    snprintf(buf, sizeof(buf), "%u", s.scoreL);
    ctx.drawStr(55 + ox, 8 + oy + scoreOffsetY, buf);
    
    snprintf(buf, sizeof(buf), "%u", s.scoreR);
    ctx.drawStr(70 + ox, 8 + oy + scoreOffsetY, buf);

    // Centre dashes
    for (int y = PongState::FIELD_TOP + 2; y < Console::H; y += 7)
        ctx.drawVLine(64 + ox, y + oy, 4);

    // Particles
    for (const auto& p : _particles) {
        if (p.life > 0) ctx.drawPixel((int)p.x + ox, (int)p.y + oy);
    }

    // ── Paddles (with Squash & Stretch) ──
    
    // Left Paddle: Bulges out to the right
    int lPadW = PongState::PAD_W + _leftHitTimer;
    int lPadH = PongState::PAD_H - (_leftHitTimer * 2);
    int lPadY = (int)s.leftY + _leftHitTimer; // Offset Y to keep it vertically centered
    ctx.drawBox(PongState::PAD_MARGIN + ox, lPadY + oy, lPadW, lPadH);

    // Right Paddle: Bulges out to the left (Subtract timer from X to pin it to the wall)
    int rPadW = PongState::PAD_W + _rightHitTimer;
    int rPadH = PongState::PAD_H - (_rightHitTimer * 2);
    int rPadY = (int)s.rightY + _rightHitTimer;
    int rPadX = Console::W - PongState::PAD_MARGIN - PongState::PAD_W - _rightHitTimer;
    ctx.drawBox(rPadX + ox, rPadY + oy, rPadW, rPadH);

    // Speed Trail
    // Multiply velocity by 1.5 or 2 to make the tail longer at higher speeds
    int tailX = (int)(s.ballPos.x - (s.ballVel.x * 1.5f));
    int tailY = (int)(s.ballPos.y - (s.ballVel.y * 1.5f));
    ctx.drawLine(s.ballPos.ix() + ox, s.ballPos.iy() + oy, tailX + ox, tailY + oy);

    // Ball
    ctx.drawDisc(s.ballPos.ix() + ox, s.ballPos.iy() + oy, PongState::BALL_R);
}

void PongPlayScene::update(Console& ctx, SceneManager& sm) {
    // If we are in hit-stop, freeze time! (Don't update ball, paddles, or timers)
if (_hitStopFrames > 0) {
    _hitStopFrames--;
    return; 
}

    // MENU1 → hard exit back to OS
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    // MENU2 / B → pause
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.push(_pause, ctx);
        return;
    }

    auto& s = _st;

    // ── Player paddle ──────────────
    if (ctx.pressed(Btn::UP))   s.leftY -= PongState::PAD_SPEED;
    if (ctx.pressed(Btn::DOWN)) s.leftY += PongState::PAD_SPEED;
    s.leftY = gclamp(s.leftY,
                     (float)PongState::FIELD_TOP,
                     (float)(PongState::FIELD_TOP + PongState::FIELD_H - PongState::PAD_H));

    // ── AI paddle ──────────────────
    _updateAI();

    // ── Timer & Ball movement ─────────────────────────────────────────
    if (_serveTimer > 0) {
        _serveTimer--;
        if (_serveTimer == 60 || _serveTimer == 30) ctx.beep(800, 30);
        if (_serveTimer == 0) ctx.beep(1200, 60);
    } else {
        s.ballPos += s.ballVel;

        // Bounce off top / bottom walls
        float topBound = (float)(PongState::FIELD_TOP + PongState::BALL_R);
        float botBound = (float)(PongState::FIELD_TOP + PongState::FIELD_H - PongState::BALL_R);
        if (s.ballPos.y <= topBound) { s.ballPos.y = topBound; if (s.ballVel.y < 0) s.ballVel.y = -s.ballVel.y; ctx.beep(300, 15); }
        if (s.ballPos.y >= botBound) { s.ballPos.y = botBound; if (s.ballVel.y > 0) s.ballVel.y = -s.ballVel.y; ctx.beep(300, 15); }

        _handlePaddleCollision(ctx);

        // Scoring
        if (s.ballPos.x < 0) {
            s.scoreR++;
            _shakeFrames = 12; // Big shake!
            ctx.sfxDeath();
            if (s.scoreR >= PongState::WIN_SCORE) { sm.replace(_gameover, ctx); return; }
            _resetBall(false); 
        }

        if (s.ballPos.x > Console::W) {
            s.scoreL++;
            _shakeFrames = 12; // Big shake!
            ctx.sfxPoint();
            if (s.scoreL >= PongState::WIN_SCORE) { _playerWon = true; sm.replace(_gameover, ctx); return; }
            _resetBall(true); 
        }
    }

    // ── Particle & Shake Physics ──
    if (_shakeFrames > 0) _shakeFrames--;
    if (_leftHitTimer > 0) _leftHitTimer--;
    if (_rightHitTimer > 0) _rightHitTimer--;

    for (auto& p : _particles) {
        if (p.life > 0) {
            p.x += p.vx;
            p.y += p.vy;
            p.vy += 0.15f; // Gravity pull on sparks
            p.life--;
        }
    }
}

void PongPlayScene::draw(Console& ctx) {
    int ox = (_shakeFrames > 0) ? random(-2, 3) : 0;
    int oy = (_shakeFrames > 0) ? random(-2, 3) : 0;
    
    // Flash the screen for the first 3 frames of a score impact
    if (_shakeFrames > 9) {
        ctx.setDrawColor(1); // White
        ctx.drawBox(0, 0, Console::W, Console::H); // Fill screen
        ctx.setDrawColor(0); // Set draw color to Black for everything else
    } else {
        ctx.setDrawColor(1); // Normal white drawing
    }
    
    drawField(ctx, ox, oy);
    
    // Draw the countdown number
    if (_serveTimer > 0) {
        int sec = (_serveTimer - 1) / 30 + 1;
        char buf[2] = { (char)('0' + sec), '\0' };
        ctx.setFont(u8g2_font_7x13B_tf);
        int w = ctx.strWidth(buf);
        
        ctx.setDrawColor(0);
        ctx.drawBox((Console::W - w) / 2 - 2, 20, w + 4, 16);
        ctx.setDrawColor(1);
        ctx.drawStr((Console::W - w) / 2, 32, buf);
    }

    ctx.setDrawColor(1);
}

// ═════════════════════════════════════════════════════════════════════════════
// PongPauseScene
// ═════════════════════════════════════════════════════════════════════════════

void PongPauseScene::onEnter(Console& ctx) {}

void PongPauseScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.pop(ctx);   // reveals PlayScene underneath
    }
}

void PongPauseScene::draw(Console& ctx) {
    // Draw the frozen game state as background with NO shake (0, 0)
    _play->drawField(ctx, 0, 0);

    // Overlay: filled box to obscure + border
    ctx.setDrawColor(0);
    ctx.drawBox(34, 22, 60, 22);
    ctx.setDrawColor(1);
    ctx.drawFrame(34, 22, 60, 22);

    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(42, 37, "PAUSED");
}

// ═════════════════════════════════════════════════════════════════════════════
// PongGameOverScene
// ═════════════════════════════════════════════════════════════════════════════

void PongGameOverScene::onEnter(Console& ctx) {
    _frame = 0;
    if (_play->playerWon()) ctx.sfxPoint();
    else                    ctx.sfxDeath();
}

void PongGameOverScene::update(Console& ctx, SceneManager& sm) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);   // fresh match, scores reset in onEnter
    }
}

void PongGameOverScene::draw(Console& ctx) {
    const PongState& s = _play->state();

    // Final scoreboard
    ctx.setFont(u8g2_font_7x13B_tf);
    char buf[8];
    snprintf(buf, sizeof(buf), "%u  %u", s.scoreL, s.scoreR);
    int w = ctx.strWidth(buf);
    ctx.drawStr((Console::W - w) / 2, 16, buf);

    ctx.drawHLine(0, PongState::FIELD_TOP - 1, Console::W);

    // Winner banner
    ctx.setDrawColor(0);
    ctx.drawBox(14, 20, 100, 26);
    ctx.setDrawColor(1);
    ctx.drawFrame(14, 20, 100, 26);
    ctx.setFont(u8g2_font_7x13B_tf);
    if (_play->playerWon()) ctx.drawStr(20, 36, "YOU  WIN!");
    else                    ctx.drawStr(22, 36, "AI  WINS!");

    // Blinking restart prompt
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(26, 56, "A: rematch  M1: menu");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// PongGame  — just wires scenes and delegates
// ═════════════════════════════════════════════════════════════════════════════

void PongGame::onEnter(Console& ctx) {
    // Wire sibling pointers so scenes can reach each other for transitions
    // and background-draw calls.  Do this ONCE here, never in scene onEnter.
    _title.setPlayScene    (&_play);
    _play .setPauseScene   (&_pause);
    _play .setGameOverScene(&_gameover);
    _pause.setPlayScene    (&_play);
    _gameover.setPlayScene (&_play);

    _sm.replace(&_title, ctx);
}

void PongGame::onExit(Console& ctx) {
    // sm.clear() has already been called; nothing to flush.
}

void PongGame::update(Console& ctx) {
    _sm.update(ctx);
}

void PongGame::draw(Console& ctx) {
    _sm.draw(ctx);
}

bool        PongGame::isRunning()   const { return !_sm.empty(); }
bool        PongGame::needsRedraw() const { return _sm.needsRedraw(); }
const char* PongGame::getName()     const { return "Pong"; }