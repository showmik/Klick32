#pragma once
// Target path: lib/SnakeGame/SnakeGame.h
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "SceneGame.h"

// ─── SnakeGame ────────────────────────────────────────────────────────────────
// Classic snake: eat apples, grow, avoid walls and yourself.
//
// Controls:
//   D-pad        → steer (2-input buffer, no 180-degree reversal)
//   B / MENU2    → pause / resume
//   A            → confirm name, restart after death
//   MENU1        → return to OS menu (from any screen; saves hi-score first)
//
// Features carried over from the original monolithic implementation:
//   • Screen wrapping (all four edges)
//   • Bonus apple  — blinking larger disc, 50 pts, 15 % spawn, timed
//   • Poison apple — X shape, −20 pts & −2 length, 20 % spawn, timed
//   • Obstacle walls added every 100-point milestone
//   • Speed increases every milestone (floor: 2 frames/step)
//   • Screen-shake on crash (15 frames)
//   • ABC-style 3-char name entry for new hi-scores
//
// Scene transition map:
//   PlayScene     ──MENU2/B──────────→ PauseScene    (push)
//   PlayScene     ──crash, new hi ──→ NameEntryScene (push)
//   PlayScene     ──crash, no hi  ──→ DeadScene      (push)
//   PlayScene     ──MENU1─────────────→ (clear → game exits)
//   PauseScene    ──MENU2/B/A ───────→ PlayScene     (pop)
//   PauseScene    ──MENU1─────────────→ (clear → game exits)
//   NameEntryScene──A (confirm) ─────→ DeadScene     (replace)
//   NameEntryScene──MENU1 ───────────→ (save + clear → game exits)
//   DeadScene     ──A / UP ──────────→ PlayScene     (replace)
//   DeadScene     ──MENU1───────────→ (clear → game exits)
//
// Stack depth never exceeds 2 (play + one overlay).
// ─────────────────────────────────────────────────────────────────────────────

// Forward declarations so each class can hold pointers to siblings.
class SnakePlayScene;
class SnakePauseScene;
class SnakeNameEntryScene;
class SnakeDeadScene;

// ─── SnakeSharedData ──────────────────────────────────────────────────────────
// Owned by SnakeGame.  All scenes receive a pointer.
// Isolates the data that must survive scene transitions.
//
// Hi-score is loaded from NVS in SnakeGame::onEnter and written back to NVS
// in NameEntryScene when the player confirms a new name.
// ─────────────────────────────────────────────────────────────────────────────
struct SnakeSharedData {
    uint32_t hiScore    = 0;
    char     hiName[4]  = "AAA";
    uint32_t lastScore  = 0;    // set by PlayScene at the moment of crash
    bool     newHiScore = false; // true when lastScore beat hiScore
};

// ─── SnakePlayScene ───────────────────────────────────────────────────────────
class SnakePlayScene : public Scene {
public:
    // Wire sibling pointers in SnakeGame::onEnter before first push.
    void setData       (SnakeSharedData*       d) { _data      = d; }
    void setEngine     (Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

    // Called by overlay scenes so they can paint the frozen field as background.
    void drawField(Console& ctx) const;

private:
    // ── Grid constants ────────────────────────────────────────────────────────
    static constexpr int      BLOCK_SIZE     = 4;
    static constexpr int      GRID_W         = 32;
    static constexpr int      GRID_H         = 14;
    static constexpr int      TOP_OFFSET     = 8;
    static constexpr int      MAX_SNAKE      = 128;
    static constexpr int      MAX_WALLS      = 20;
    static constexpr uint8_t  START_SPEED    = 6;
    static constexpr uint32_t BONUS_POINTS   = 50;
    static constexpr uint16_t BONUS_DURATION = 40;
    static constexpr uint16_t POISON_DURATION = 50;

    enum class SparkType { NORMAL, POISON, BONUS };

    void _spawnSparks(int gridX, int gridY, SparkType type);

    // ── Round state (reset on every new game) ─────────────────────────────────
    enum class Dir : uint8_t { UP, DOWN, LEFT, RIGHT };

    Dir      _dir        = Dir::RIGHT;
    Dir      _inputQueue[2] = {};
    uint8_t  _queueLen   = 0;

    int      _len        = 0;
    int      _sx[MAX_SNAKE] = {};
    int      _sy[MAX_SNAKE] = {};

    int      _wx[MAX_WALLS] = {};
    int      _wy[MAX_WALLS] = {};
    int      _numWalls   = 0;

    int      _ax = 0, _ay = 0;   // normal apple

    int      _bx = 0, _by = 0;   // bonus apple
    bool     _bonusActive  = false;
    uint16_t _bonusTimer   = 0;

    int      _px = 0, _py = 0;   // poison apple
    bool     _poisonActive = false;
    uint16_t _poisonTimer  = 0;

    uint32_t _score      = 0;
    uint8_t  _moveTimer  = 0;
    uint8_t  _speed      = START_SPEED;

    // ── Wired siblings ────────────────────────────────────────────────────────
    SnakeSharedData*     _data      = nullptr;
    Camera*              _camera    = nullptr;
    ParticleManager*    _particles = nullptr;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();
    bool _isOccupied(int x, int y) const;
    void _spawnApple();
    void _spawnBonus();
    void _spawnPoison();
    void _spawnWall();
    void _pushInput(Dir d);
};

// ─── SnakePauseScene ─────────────────────────────────────────────────────────
class SnakePauseScene : public Scene {
public:
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
};

// ─── SnakeNameEntryScene ─────────────────────────────────────────────────────
// Shown when the player sets a new hi-score.
// Saves to NVS and transitions to DeadScene on confirm.
class SnakeNameEntryScene : public Scene {
public:
    void setData     (SnakeSharedData*  d) { _data = d; }
    void setEngine   (Camera* cam)         { _camera = cam; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

private:
    SnakeSharedData* _data    = nullptr;
    Camera*          _camera  = nullptr;

    char    _currName[4] = "AAA";
    uint8_t _nameIdx     = 0;
    uint8_t _frame       = 0;

    void _saveToNVS(Console& ctx);
};

// ─── SnakeDeadScene ───────────────────────────────────────────────────────────
class SnakeDeadScene : public Scene {
public:
    void setData     (SnakeSharedData*  d) { _data = d; }
    void setEngine   (Camera* cam)         { _camera = cam; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

private:
    SnakeSharedData* _data    = nullptr;
    Camera*          _camera  = nullptr;

    uint8_t _frame       = 0;
};

// ─── SnakeGame ────────────────────────────────────────────────────────────────
class SnakeGame : public SceneGame<SnakeSharedData> {
public:
    void onEnter(Console& ctx) override;
    const char*    getName()     const override;
    const uint8_t* getCoverArt() const override;

private:
    SnakePlayScene       _play;
    SnakePauseScene      _pause;
    SnakeNameEntryScene  _nameEntry;
    SnakeDeadScene       _dead;
};