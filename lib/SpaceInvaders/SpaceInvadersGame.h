#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"

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
    void setPlayScene(SIPlayScene* p) { _play = p; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    SIPlayScene* _play = nullptr;
    uint8_t      _frame = 0;
};

// ─── SIPlayScene ───────────────────────────────────────────────────────────
class SIPlayScene : public Scene {
public:
    void setData      (SISharedData* d)    { _data = d; }
    void setPauseScene(SIPauseScene* p)    { _pause = p; }
    void setDeadScene (SIGameOverScene* d) { _gameover = d; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
    void drawField(Console& ctx) const;

private:
    static constexpr int ALIEN_ROWS = 3;
static constexpr int ALIEN_COLS = 8; 
    static constexpr int MAX_EBULLETS = 3;
    
    SISharedData*    _data     = nullptr;
    SIPauseScene*    _pause    = nullptr;
    SIGameOverScene* _gameover = nullptr;

    float _playerX = 60.0f;
    
    // Player Bullet
    float _pbX = 0;
    float _pbY = 0;
    bool  _pbActive = false;

    // Swarm
    bool  _aliens[ALIEN_ROWS][ALIEN_COLS];
    int   _aliensAlive = 0;
    float _swarmX = 10.0f;
    float _swarmY = 10.0f;
    float _swarmVX = 1.0f;
    uint8_t _animFrame = 0;
    uint8_t _moveTimer = 0;
    uint8_t _moveDelay = 30; // Gets faster as aliens die

    // Enemy Bullets
    struct EBullet { float x, y; bool active; };
    EBullet _eBullets[MAX_EBULLETS];

    uint8_t _shakeFrames = 0;
    uint8_t _respawnTimer = 0;

    void _initLevel();
    void _checkCollisions(Console& ctx, SceneManager& sm);
};

// ─── SIPauseScene ──────────────────────────────────────────────────────────
class SIPauseScene : public Scene {
public:
    void setPlayScene(SIPlayScene* p) { _play = p; }
    void onEnter(Console& ctx) override {}
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    SIPlayScene* _play = nullptr;
};

// ─── SIGameOverScene ───────────────────────────────────────────────────────
class SIGameOverScene : public Scene {
public:
    void setData     (SISharedData* d) { _data = d; }
    void setPlayScene(SIPlayScene* p)  { _play = p; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    SISharedData* _data = nullptr;
    SIPlayScene*  _play = nullptr;
    uint8_t       _frame = 0;
};

// ─── SpaceInvadersGame ─────────────────────────────────────────────────────
class SpaceInvadersGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    void update (Console& ctx) override;
    void draw   (Console& ctx) override;
    bool           isRunning() const override;
    const char*    getName()   const override;

private:
    SISharedData    _data;
    SceneManager    _sm;
    SITitleScene    _title;
    SIPlayScene     _play;
    SIPauseScene    _pause;
    SIGameOverScene _gameover;
};