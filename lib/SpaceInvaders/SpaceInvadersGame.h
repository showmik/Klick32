#pragma once
#include "SceneGame.h"
#include "GameUtils.h"
#include "Sprite.h"

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
    void setEngine    (Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
    void drawField(Console& ctx) const;

private:
    static constexpr int ALIEN_ROWS = 3;
    static constexpr int ALIEN_COLS = 8; 
    static constexpr int MAX_EBULLETS = 3;
    static constexpr int MAX_STARS = 40;
    
    struct Star { float x, y, speed; };

    SISharedData*    _data     = nullptr;
    Camera*          _camera   = nullptr;
    ParticleManager* _particles = nullptr;

    Star _stars[MAX_STARS];

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
    void setEngine   (ParticleManager* anim) { _particles = anim; }
    
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    SISharedData* _data = nullptr;
    SIPlayScene*  _play = nullptr;
    ParticleManager* _particles = nullptr;
    uint8_t       _frame = 0;
};

// ─── SpaceInvadersGame ─────────────────────────────────────────────────────
class SpaceInvadersGame : public SceneGame<SISharedData> {
public:
    void onEnter(Console& ctx) override;
    const char*    getName()   const override;
    const uint8_t* getCoverArt() const override;

private:
    SITitleScene     _title;
    SIPlayScene      _play;
    SIPauseScene     _pause;
    SIGameOverScene  _gameover;
};
