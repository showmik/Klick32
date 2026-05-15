#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Sprite.h"
#include "Camera.h"
#include "AnimationManager.h"

// Forward declarations
class SIPlayScene;
class SIPauseScene;
class SIGameOverScene;

struct SISharedData {
    uint32_t score   = 0;
    uint32_t hiScore = 0;
    int8_t   lives   = 3;
};

// ─── SITitleScene ──────────────────────────────────────────────────────────
class SITitleScene : public Scene {
public:
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    uint8_t      _frame = 0;
};

// ─── SIPlayScene ───────────────────────────────────────────────────────────
class SIPlayScene : public Scene {
public:
    void setData      (SISharedData* d)    { _data = d; }
    void setEngine    (Camera* cam, AnimationManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
    void drawField(Console& ctx) const;

private:
    static constexpr int ALIEN_ROWS = 3;
    static constexpr int ALIEN_COLS = 8; 
    static constexpr int MAX_EBULLETS = 3;
    static constexpr int MAX_STARS = 40;
    
    SISharedData*    _data     = nullptr;
    Camera*          _camera   = nullptr;
    AnimationManager* _particles = nullptr;

    Sprite _player;
    Sprite _pb; // Player Bullet
    Sprite _eBullets[MAX_EBULLETS];
    Sprite _aliens[ALIEN_ROWS][ALIEN_COLS];

    int   _aliensAlive = 0;
    int   _wave = 0;
    float _swarmX = 10.0f;
    float _swarmY = 10.0f;
    float _swarmVX = 1.0f;
    uint8_t _animFrame = 0;
    uint8_t _moveTimer = 0;
    uint8_t _moveDelay = 30; // Gets faster as aliens die

    uint8_t _respawnTimer = 0;

    void _initLevel();
    void _checkCollisions(Console& ctx, SceneManager& sm);
};

// ─── SIPauseScene ──────────────────────────────────────────────────────────
class SIPauseScene : public Scene {
public:
    void onEnter(Console& ctx) override {}
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
};

// ─── SIGameOverScene ───────────────────────────────────────────────────────
class SIGameOverScene : public Scene {
public:
    void setData     (SISharedData* d) { _data = d; }
    void setPlayScene(SIPlayScene* p)  { _play = p; }
    void setEngine   (AnimationManager* anim) { _particles = anim; }
    
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    SISharedData* _data = nullptr;
    SIPlayScene*  _play = nullptr;
    AnimationManager* _particles = nullptr;
    uint8_t       _frame = 0;
};

// ─── SpaceInvadersGame ─────────────────────────────────────────────────────
class SpaceInvadersGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    void update (Console& ctx, float dt) override;
    void draw   (Console& ctx) override;
    bool           isRunning() const override;
    const char*    getName()   const override;
    const uint8_t* getCoverArt() const override;

private:
    SISharedData     _data;
    SceneManager     _sm;
    Camera           _camera;
    AnimationManager _particles;
    SITitleScene     _title;
    SIPlayScene      _play;
    SIPauseScene     _pause;
    SIGameOverScene  _gameover;
};
ameOverScene  _gameover;
};
