#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include "ParticleManager.h"

// ─── Shared Game State ──────────────────────────────────────────────────────
struct BBSharedData {
    uint32_t score   = 0;
    uint32_t hiScore = 0;
    int      lives   = 3;
    int      level   = 1;
};

// ─── Enums & Structs ────────────────────────────────────────────────────────
enum class BrickType : uint8_t { NONE = 0, NORMAL = 1, HARD = 2, SOLID = 3 };
enum class PowerUpType : uint8_t { EXPAND, SHRINK, CATCH, MULTIBALL, FIREBALL, LIFE, LASER };

struct BBBrick {
    BrickType type = BrickType::NONE;
    int hp = 0;
    bool active = false;
};

struct BBBall {
    float x = 0, y = 0;
    float vx = 0, vy = 0;
    bool active = false;
    bool sticky = false;
    float stuckOffset = 0; // relative to paddle center
    bool fireball = false;
};

struct BBPowerUp {
    float x = 0, y = 0;
    PowerUpType type;
    bool active = false;
};

struct BBLaser {
    float x = 0, y = 0;
    bool active = false;
};

// Forward Declarations
class BBPlayScene;
class BBPauseScene;
class BBGameOverScene;

// ─── BBTitleScene ───────────────────────────────────────────────────────────
class BBTitleScene : public Scene {
public:
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    uint8_t _frame = 0;
};

// ─── BBPlayScene ────────────────────────────────────────────────────────────
class BBPlayScene : public Scene {
public:
    void setData   (BBSharedData* d) { _data = d; }
    void setEngine (Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
    void drawField(Console& ctx) const;

private:
    // Layout
    static constexpr int COLS = 15;
    static constexpr int ROWS = 5;
    static constexpr int BRICK_W = 8;
    static constexpr int BRICK_H = 4;
    static constexpr int GRID_OX = 4;
    static constexpr int GRID_OY = 12;
    
    // Config
    static constexpr int MAX_BALLS = 3;
    static constexpr int MAX_POWERUPS = 5;
    static constexpr int MAX_LASERS = 10;
    static constexpr float INIT_SPEED = 1.3f;
    static constexpr float MAX_SPEED = 2.4f;
    static constexpr float PAD_Y = 60.0f;

    BBSharedData*     _data      = nullptr;
    Camera*           _camera    = nullptr;
    ParticleManager* _particles = nullptr;

    BBBrick   _bricks[ROWS][COLS];
    BBBall    _balls[MAX_BALLS];
    BBPowerUp _powerUps[MAX_POWERUPS];
    BBLaser   _lasers[MAX_LASERS];

    float _padX = 0;
    float _padW = 20.0f;
    float _padSpeed = 2.5f;
    bool  _stickyPaddle = false;

    char _msg[32] = "";
    uint8_t _msgTimer = 0;
    int _bricksLeft = 0;
    int _combo = 1;
    bool _levelClearPause = false;
    uint8_t _clearTimer = 0;
    
    bool _laserActive = false;
    int _laserTimer = 0;

    void _generateLevel();
    void _spawnPowerUp(float x, float y);
    void _applyPowerUp(PowerUpType type, Console& ctx);
    void _resetBall(bool serve);
    void _normalizeBallVelocity(BBBall& b, float speedTarget);
    bool _checkBallBrick(BBBall& ball, BBBrick& brick, int bx, int by, Console& ctx);
};

// ─── BBPauseScene ───────────────────────────────────────────────────────────
class BBPauseScene : public Scene {
public:
    void onEnter(Console& ctx) override {}
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
};

// ─── BBGameOverScene ────────────────────────────────────────────────────────
class BBGameOverScene : public Scene {
public:
    void setData(BBSharedData* d) { _data = d; }
    void setPlayScene(BBPlayScene* p) { _play = p; }
    
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    BBSharedData* _data = nullptr;
    BBPlayScene*  _play = nullptr;
    uint8_t       _frame = 0;
};

// ─── BrickBreakerGame ───────────────────────────────────────────────────────
class BrickBreakerGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    void update (Console& ctx, float dt) override;
    void draw   (Console& ctx) override;
    bool           isRunning() const override;
    const char*    getName()   const override;
    const uint8_t* getCoverArt() const override;

private:
    BBSharedData      _data;
    SceneManager      _sm;
    Camera            _camera;
    ParticleManager  _particles;
    
    BBTitleScene      _title;
    BBPlayScene       _play;
    BBPauseScene      _pause;
    BBGameOverScene   _gameover;
};