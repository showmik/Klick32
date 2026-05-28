#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Sprite.h"
#include "Camera.h"
#include "ParticleManager.h"

#include "SceneGame.h"

class DinoPlayScene;
class DinoPauseScene;
class DinoDeadScene;

// ─── Shared Game State ───────────────────────────────────────────────────────
struct DinoSharedData {
    uint32_t score   = 0;
    uint32_t hiScore = 0;
    float    speed   = 0.0f;
};

// ─── DinoTitleScene ──────────────────────────────────────────────────────────
class DinoTitleScene : public Scene {
public:
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

private:
    uint8_t        _frame = 0;
};

// ─── DinoPlayScene ───────────────────────────────────────────────────────────
class DinoPlayScene : public Scene {
public:
    void setData      (DinoSharedData* d) { _data = d; }
    void setEngine    (Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

    void drawField(Console& ctx, bool isDead) const;

    float getSpeed() const { return _speed; }

private:
    Camera*           _camera    = nullptr;
    ParticleManager* _particles = nullptr;
    enum class ObstacleKind : uint8_t { CACTUS_SMALL, CACTUS_LARGE, PTERO_LOW, PTERO_HIGH };

    struct Obstacle {
        float        x;
        bool         active;
        ObstacleKind kind;
        uint8_t      animFrame;
        uint8_t      animTimer;
    };

    struct Cloud { Vec2 pos; };
    struct Dust  { Vec2 pos; uint8_t life; };
    struct Sweat { Vec2 pos; uint8_t life; };

    // ── Constants ─────────────────────────────────────────────────────────────
    static constexpr int      GROUND_Y           = 52;
    static constexpr int      DINO_X             = 10;
    static constexpr int      DINO_W             = 16;
    static constexpr int      DINO_H             = 16;
    static constexpr int      DUCK_H             = 8;
    static constexpr int      CACTUS_H           = 16;
    static constexpr int      SMALL_W            = 8;
    static constexpr int      LARGE_W            = 12;
    static constexpr uint8_t  MAX_OBS            = 2;
    static constexpr int      PTERO_W            = 16;
    static constexpr int      PTERO_H            = 8;
    static constexpr int      PTERO_LOW_Y        = 36;
    static constexpr int      PTERO_HIGH_Y       = 18;
    static constexpr uint8_t  PTERO_ANIM_RATE    = 8;
    static constexpr uint32_t PTERO_MIN_SCORE    = 200;
    static constexpr uint8_t  PTERO_W_WEIGHT     = 1;
    static constexpr uint8_t  CACTUS_W_WEIGHT    = 3;
    static constexpr uint8_t  MAX_CLOUDS         = 3;
    static constexpr uint8_t  MAX_DUST           = 4;
    static constexpr uint8_t  MAX_SWEAT          = 3;
    static constexpr float    GRAVITY            = 0.55f;
    static constexpr float    JUMP_VY            = -8.0f;
    static constexpr float    INIT_SPEED         = 2.5f;
    static constexpr float    MAX_SPEED          = 7.5f;
    static constexpr float    SPEED_INC          = 0.002f;
    static constexpr int      MIN_GAP            = 55;
    static constexpr int      MAX_GAP            = 120;
    static constexpr uint32_t SCORE_MILESTONE    = 100;
    static constexpr uint8_t  FLASH_FRAMES       = 60;
    static constexpr uint8_t  COYOTE_FRAMES      = 6;
    static constexpr uint8_t  JUMP_BUFFER_FRAMES = 8;

    // ── Members ───────────────────────────────────────────────────────────────
    DinoSharedData* _data   = nullptr;

    bool       _isDucking     = false;
    float      _dinoY         = 0.0f;
    float      _dinoVY        = 0.0f;
    bool       _onGround      = true;
    uint8_t    _coyoteFrames  = 0;
    uint8_t    _jumpBuffer    = 0;
    uint32_t   _lastMilestone = 0;
    uint8_t    _flashTimer    = 0;
    uint8_t    _blinkTimer    = 0;
    float      _speed         = INIT_SPEED;
    Obstacle   _obs[MAX_OBS]       = {};
    Cloud      _clouds[MAX_CLOUDS] = {};
    Dust       _dust[MAX_DUST]     = {};
    Sweat      _sweat[MAX_SWEAT]   = {};
    uint32_t   _frameCnt  = 0;
    uint8_t    _animTimer = 0;
    uint8_t    _animFrame = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();
    void _spawnObsIfNeeded();
    bool _checkCollision(const Obstacle& o) const;
    static int  _obsWidth(ObstacleKind k);
    static int  _obsTopY (ObstacleKind k);
    static bool _isPtero (ObstacleKind k);
    void _drawCloud(Console& ctx, int x, int y) const;
};

// ─── DinoPauseScene ──────────────────────────────────────────────────────────
class DinoPauseScene : public Scene {
public:
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
};

// ─── DinoDeadScene ───────────────────────────────────────────────────────────
class DinoDeadScene : public Scene {
public:
    void setData     (DinoSharedData* d) { _data = d; }
    void setEngine   (Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }
    void setPlayScene(DinoPlayScene* p) { _play = p; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

private:
    DinoSharedData*   _data      = nullptr;
    Camera*           _camera    = nullptr;
    ParticleManager* _particles = nullptr;
    DinoPlayScene*    _play      = nullptr;
    uint8_t           _frame     = 0;
};

// ─── DinoGame ────────────────────────────────────────────────────────────────
class DinoGame : public SceneGame<DinoSharedData> {
public:
    void onEnter(Console& ctx) override;
    void onExit(Console& ctx) override;
    
    const char*    getName()   const override;
    const uint8_t* getIcon()   const override;
    const uint8_t* getCoverArt() const override;

private:
    DinoTitleScene _title;
    DinoPlayScene  _play;
    DinoPauseScene _pause;
    DinoDeadScene  _dead;
};