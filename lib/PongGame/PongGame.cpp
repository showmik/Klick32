// Target path: lib/PongGame/PongGame.cpp
#include "PongGame.h"
#include "GameRegistry.h"
#include "PongSprites.h"
#include "CommonScreens.h"

// ═════════════════════════════════════════════════════════════════════════════
// PongTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void PongTitleScene::onEnter(Console& ctx) {
    _frame = 0;
}

void PongTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;


    if (ctx.justPressed(Btn::A) ) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Map to PlayScene
    }
}

void PongTitleScene::draw(Console& ctx) {
    Screens::drawTitle(ctx, "PONG");
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
    for (int i = 0; i < 5; i++) {
        _particles->spawnPixel(x, y, dirX * (random(15, 35) / 10.0f), (random(-25, 25) / 10.0f), random(8, 15));
    }
}

void PongPlayScene::_handlePaddleCollision(Console& ctx) {
    auto& s = _st;

    // Player paddle
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
        
        if (_camera) _camera->shake(3);
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
        
        if (_camera) _camera->shake(3);
        _rightHitTimer = 4;
        _spawnSparks(s.ballPos.x, s.ballPos.y, -1.0f);
        ctx.beep(600 + abs((int)s.ballVel.y) * 50, 30); 
    }

    s.ballVel.x = gclamp(s.ballVel.x, -PongState::MAX_SPEED, PongState::MAX_SPEED);
    s.ballVel.y = gclamp(s.ballVel.y, -PongState::MAX_SPEED, PongState::MAX_SPEED);
}

// Helper used by both PlayScene and PauseScene (the latter draws background).
void PongPlayScene::drawField(Console& ctx) const {
    const PongState& s = _st;
    
    ctx.setCamera(nullptr); // UI first

    // HUD line + scores
    ctx.setFont(u8g2_font_5x7_tf);
    
    // If the serve timer is high (just scored), jump the numbers up by 3 pixels
    int scoreOffsetY = (_serveTimer > 70) ? -3 : 0; 

    ctx.drawPrintf(55, 8 + scoreOffsetY, "%u", s.scoreL);
    ctx.drawPrintf(70, 8 + scoreOffsetY, "%u", s.scoreR);

    ctx.setCamera(_camera);

    // Centre dashes
    for (int y = PongState::FIELD_TOP + 2; y < Console::H; y += 7)
        ctx.drawVLine(64, y, 4);

    // Particles
    _particles->draw(ctx);

    // ── Paddles (with Squash & Stretch) ──
    
    // Left Paddle: Bulges out to the right
    int lPadW = PongState::PAD_W + _leftHitTimer;
    int lPadH = PongState::PAD_H - (_leftHitTimer * 2);
    int lPadY = (int)s.leftY + _leftHitTimer; // Offset Y to keep it vertically centered
    ctx.drawBox(PongState::PAD_MARGIN, lPadY, lPadW, lPadH);

    // Right Paddle: Bulges out to the left (Subtract timer from X to pin it to the wall)
    int rPadW = PongState::PAD_W + _rightHitTimer;
    int rPadH = PongState::PAD_H - (_rightHitTimer * 2);
    int rPadY = (int)s.rightY + _rightHitTimer;
    int rPadX = Console::W - PongState::PAD_MARGIN - PongState::PAD_W - _rightHitTimer;
    ctx.drawBox(rPadX, rPadY, rPadW, rPadH);

    // Speed Trail
    // Multiply velocity by 1.5 or 2 to make the tail longer at higher speeds
    int tailX = (int)(s.ballPos.x - (s.ballVel.x * 1.5f));
    int tailY = (int)(s.ballPos.y - (s.ballVel.y * 1.5f));
    ctx.drawLine(s.ballPos.ix(), s.ballPos.iy(), tailX, tailY);

    // Ball
    ctx.drawDisc(s.ballPos.ix(), s.ballPos.iy(), PongState::BALL_R);

    ctx.setCamera(nullptr);
}

void PongPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    // If we are in hit-stop, freeze time! (Don't update ball, paddles, or timers)
if (_hitStopFrames > 0) {
    _hitStopFrames--;
    return; 
}

    // MENU1 → hard exit back to OS

    // MENU2 / B → pause
    if ( ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::PAUSE);
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
            if (_camera) _camera->shake(12); // Big shake!
            ctx.sfxDeath();
            if (s.scoreR >= PongState::WIN_SCORE) { sm.emit(ctx, Event::GAME_OVER); return; }
            _resetBall(false); 
        }

        if (s.ballPos.x > Console::W) {
            s.scoreL++;
            if (_camera) _camera->shake(12); // Big shake!
            ctx.sfxPoint();
            if (s.scoreL >= PongState::WIN_SCORE) { _playerWon = true; sm.emit(ctx, Event::GAME_OVER); return; }
            _resetBall(true); 
        }
    }

    // ── Particle & Shake Physics ──
    if (_leftHitTimer > 0) _leftHitTimer--;
    if (_rightHitTimer > 0) _rightHitTimer--;
}

void PongPlayScene::draw(Console& ctx) {
    // Flash the screen for the first 3 frames of a score impact
    if (_serveTimer > 87) {
        ctx.setDrawColor(1); // White
        ctx.drawBox(0, 0, Console::W, Console::H); // Fill screen
        ctx.setDrawColor(0); // Set draw color to Black for everything else
    } else {
        ctx.setDrawColor(1); // Normal white drawing
    }
    
    drawField(ctx);
    
    // Draw the countdown number
    if (_serveTimer > 0) {
        int sec = (_serveTimer - 1) / 30 + 1;
        char buf[2] = { (char)('0' + sec), '\0' };
        ctx.setFont(u8g2_font_7x13B_tf);
        int w = ctx.strWidth(buf);
        
        ctx.setDrawColor(0);
        ctx.drawBox((Console::W - w) / 2 - 2, 20, w + 4, 16);
        ctx.setDrawColor(1);
        ctx.drawStrCentered(32, buf);
    }

    ctx.setDrawColor(1);
}

// ═════════════════════════════════════════════════════════════════════════════
// PongPauseScene
// ═════════════════════════════════════════════════════════════════════════════

void PongPauseScene::onEnter(Console& ctx) {}

void PongPauseScene::update(Console& ctx, SceneManager& sm, float dt) {

    if ( ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
    }
}

void PongPauseScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    Screens::drawPauseOverlay(ctx);
}

// ═════════════════════════════════════════════════════════════════════════════
// PongGameOverScene
// ═════════════════════════════════════════════════════════════════════════════

void PongGameOverScene::onEnter(Console& ctx) {
    _frame = 0;
    if (*_playerWon) ctx.sfxPoint();
    else             ctx.sfxDeath();
}

void PongGameOverScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1);   // fresh match, scores reset in onEnter
    }
}

void PongGameOverScene::draw(Console& ctx) {
    ctx.setCamera(nullptr);
    
    // Clear background
    ctx.setDrawColor(0);
    ctx.drawBox(0, 0, Console::W, Console::H);
    
    ctx.setDrawColor(1);
    
    // Draw court line
    for (int y = 0; y < Console::H; y += 7) {
        ctx.drawVLine(64, y, 4);
    }
    
    // Final scoreboard
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawPrintf(32, 20, "%u", _st->scoreL);
    ctx.drawPrintf(88, 20, "%u", _st->scoreR);

    // Winner banner
    ctx.setDrawColor(0);
    ctx.drawBox(14, 26, 100, 20);
    ctx.setDrawColor(1);
    ctx.drawFrame(14, 26, 100, 20);
    
    ctx.setFont(u8g2_font_7x13B_tf);
    if (*_playerWon) {
        ctx.drawStrCentered(40, "YOU WIN!");
    } else {
        ctx.drawStrCentered(40, "AI WINS!");
    }

    // Blinking restart prompt
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStrCentered(58, "A: rematch  M1: menu");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// PongGame  — just wires scenes and delegates
// ═════════════════════════════════════════════════════════════════════════════

void PongGame::onEnter(Console& ctx) { ctx.setCPUSpeed(80);
    _play.setEngine(&_camera, &_particles);
    
    _gameover.setState(_play.getStatePtr(), _play.getPlayerWonPtr());

    useDefaultEvents(&_pause, &_gameover);
    _sm.onEvent(Event::CUSTOM_1,  SceneManager::REPLACE, &_play); // Start/Restart

    _sm.replace(&_title, ctx);
}

bool        PongGame::needsRedraw() const { return _sm.needsRedraw(); }
const char* PongGame::getName()     const { return "Pong"; }
const uint8_t* PongGame::getCoverArt() const { return spr_pong_cover; }

REGISTER_GAME(PongGame);
