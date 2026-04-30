#pragma once
// Target path: lib/PongGame/PongGame.h
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"

// ─── PongGame ─────────────────────────────────────────────────────────────────
// Classic two-paddle Pong: player (left) vs AI (right).
// First to WIN_SCORE points wins.
//
// Controls:
//   UP / DOWN   → move left paddle
//   MENU2 / B   → pause / resume
//   A           → confirm / restart
//   MENU1       → return to OS menu (from any screen)
//
// This file is also the reference example for the SceneManager pattern.
// Each screen (title, play, pause, game over) is its own Scene subclass.
// PongGame owns all four instances and the SceneManager; it just delegates.
//
// Scene transition map:
//   TitleScene    ──A──────────────→ PlayScene   (replace)
//   PlayScene     ──MENU2/B─────→  PauseScene  (push)
//   PlayScene     ──score ≥ WIN──→  GameOverScene (replace)
//   PlayScene     ──MENU1────────→  (clear → game exits)
//   PauseScene    ──MENU2/B─────→  PlayScene   (pop)
//   PauseScene    ──MENU1────────→  (clear → game exits)
//   GameOverScene ──A────────────→  PlayScene   (replace)
//   GameOverScene ──MENU1────────→  (clear → game exits)
// ─────────────────────────────────────────────────────────────────────────────

// Forward-declare scene classes so PongGame can declare members without
// the full definitions available yet.
class PongPlayScene;
class PongPauseScene;
class PongGameOverScene;
class PongTitleScene;

// ─── Shared game state ────────────────────────────────────────────────────────
// Owned by PlayScene; read by PauseScene (draws background) and GameOverScene.
// All geometry constants live here so scenes never disagree on layout.
struct PongState {
    // Ball
    Vec2    ballPos;
    Vec2    ballVel;
    // Paddle top-left y positions
    float   leftY  = 0.0f;
    float   rightY = 0.0f;
    // Scores
    uint8_t scoreL = 0;
    uint8_t scoreR = 0;

    // ── Layout ───────────────────────────────────────────────────────────────
    static constexpr int   FIELD_TOP   = 10;
    static constexpr int   FIELD_H     = Console::H - FIELD_TOP;
    static constexpr int   PAD_W       = 3;
    static constexpr int   PAD_H       = 14;
    static constexpr int   PAD_MARGIN  = 4;
    static constexpr int   BALL_R      = 2;

    // ── Tuning ───────────────────────────────────────────────────────────────
    static constexpr uint8_t WIN_SCORE  = 5;
    static constexpr float   BALL_SPEED = 2.8f;
    static constexpr float   PAD_SPEED  = 3.2f;  // Increased from 2.2f for snappier control
    static constexpr float   AI_SPEED   = 1.7f;
    static constexpr float   MAX_SPEED  = 6.5f;
};

// ─── Scene declarations ───────────────────────────────────────────────────────

class PongTitleScene : public Scene {
public:
    void setPlayScene(PongPlayScene* p) { _play = p; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;

private:
    PongPlayScene* _play  = nullptr;
    uint8_t        _frame = 0;          // drives blink animation
};

class PongPlayScene : public Scene {
public:
    void setPauseScene   (PongPauseScene*    p) { _pause    = p; }
    void setGameOverScene(PongGameOverScene* g) { _gameover = g; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;

    // We've moved drawing here so it can access particles and screen shake
    void drawField(Console& ctx, int ox, int oy) const;

    const PongState& state()     const { return _st; }
    bool             playerWon() const { return _playerWon; }

private:
    PongState          _st;
    PongPauseScene*    _pause     = nullptr;
    PongGameOverScene* _gameover  = nullptr;
    bool               _playerWon = false;
    uint8_t            _serveTimer = 0;

    // ── Juice / VFX ──────────────────────────────────────────────────────────
    struct Particle { float x, y, vx, vy; uint8_t life; };
    static constexpr uint8_t MAX_PARTICLES = 15;
    Particle _particles[MAX_PARTICLES] = {};
    uint8_t _shakeFrames = 0;
    uint8_t _leftHitTimer = 0;   
    uint8_t _rightHitTimer = 0;
    uint8_t _rallyCount = 0;
    uint8_t _hitStopFrames = 0;

    void _resetBall(bool serveLeft);
    void _updateAI();
    void _handlePaddleCollision(Console& ctx); // Added ctx for audio
    void _spawnSparks(float x, float y, float dirX);
};

class PongPauseScene : public Scene {
public:
    // Needs play scene to draw the game background behind the overlay.
    void setPlayScene(PongPlayScene* p) { _play = p; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;

private:
    PongPlayScene* _play = nullptr;
};

class PongGameOverScene : public Scene {
public:
    void setPlayScene(PongPlayScene* p) { _play = p; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;

private:
    PongPlayScene* _play  = nullptr;
    uint8_t        _frame = 0;
};

// ─── PongGame ─────────────────────────────────────────────────────────────────

class PongGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    void update (Console& ctx) override;
    void draw   (Console& ctx) override;
    bool           isRunning()   const override;
    bool           needsRedraw() const override;
    const char*    getName()     const override;

private:
    SceneManager       _sm;
    PongTitleScene     _title;
    PongPlayScene      _play;
    PongPauseScene     _pause;
    PongGameOverScene  _gameover;
};