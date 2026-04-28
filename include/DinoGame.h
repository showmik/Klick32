#pragma once
#include "GameBase.h"

// ─── DinoGame ─────────────────────────────────────────────────────────────────
// Chrome-style endless runner.
//
// Controls (in-game):
//   UP  / A     → Jump
//   DOWN / B    → Duck  (only on ground; cancels jump input)
//   MENU1       → Return to OS menu
//
// Obstacles:
//   Small / large cactus  → jump over
//   Pterodactyl (low)     → jump over  (appears after score 200)
//   Pterodactyl (high)    → duck under (appears after score 200)
//
// Score milestones every 100 pts: brief flash + beep.
class DinoGame : public GameBase {
public:
    void onEnter() override;
    void onExit()  override;
    void update(InputManager& input, Sound& sound) override;
    void draw(U8G2& disp) override;
    bool isRunning()         const override;
    const char* getName()    const override;
    const uint8_t* getIcon() const override;

private:
    // ── Internal state machine ────────────────────────────────────────────────
    enum class DinoState { RUNNING, DEAD };

    struct Obstacle {
        float   x;
        bool    active;
        bool    large;          // true = 12 px wide, false = 8 px wide
    };

    struct Pterodactyl {
        float    x;
        bool     active;
        uint8_t  heightIdx;     // 0 = low (must jump), 1 = high (must duck)
        uint8_t  animFrame;
        uint32_t animTimer;
    };

    struct Cloud {
        float  x;
        int8_t y;
    };

    // ── Game constants ────────────────────────────────────────────────────────
    static constexpr int      GROUND_Y          = 52;
    static constexpr int      DINO_X            = 10;
    static constexpr int      DINO_W            = 16;
    static constexpr int      DINO_H            = 16;   // standing sprite height
    static constexpr int      DUCK_H            = 8;    // ducking sprite height
    static constexpr int      CACTUS_H          = 16;
    static constexpr int      SMALL_W           = 8;
    static constexpr int      LARGE_W           = 12;
    static constexpr uint8_t  MAX_OBS           = 2;

    static constexpr int      PTERO_W           = 16;
    static constexpr int      PTERO_H           = 8;
    static constexpr int      PTERO_LOW_Y       = 36;   // low flier  — jump over
    static constexpr int      PTERO_HIGH_Y      = 18;   // high flier — duck under
    static constexpr uint8_t  MAX_PTERO         = 1;
    static constexpr uint32_t PTERO_MIN_SCORE   = 200;  // unlocked after this score

    static constexpr uint8_t  MAX_CLOUDS        = 3;

    static constexpr float    GRAVITY           = 0.55f;
    static constexpr float    JUMP_VY           = -8.0f;
    static constexpr float    INIT_SPEED        = 2.5f;
    static constexpr float    MAX_SPEED         = 7.5f;
    static constexpr float    SPEED_INC         = 0.002f;

    static constexpr int      MIN_GAP           = 55;
    static constexpr int      MAX_GAP           = 120;

    static constexpr uint32_t SCORE_MILESTONE   = 100;  // flash + beep interval
    static constexpr uint8_t  FLASH_FRAMES      = 12;   // frames the flash lasts

    // ── Member variables ──────────────────────────────────────────────────────
    DinoState    _state         = DinoState::RUNNING;
    bool         _running       = false;
    bool         _isDucking     = false;

    float        _dinoY         = 0.0f;
    float        _dinoVY        = 0.0f;
    bool         _onGround      = true;

    uint32_t     _score         = 0;
    uint32_t     _hiScore       = 0;   // persists across rounds until power-off
    uint32_t     _lastMilestone = 0;   // last milestone score that triggered flash
    uint8_t      _flashTimer    = 0;   // counts down; >0 = score area flashing

    float        _speed         = INIT_SPEED;

    Obstacle     _obs[MAX_OBS]          = {};
    Pterodactyl  _ptero[MAX_PTERO]      = {};
    Cloud        _clouds[MAX_CLOUDS]    = {};

    uint32_t     _frameCnt  = 0;
    uint32_t     _animTimer = 0;
    uint8_t      _animFrame = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();
    void _spawnObsIfNeeded();
    void _spawnPteroIfNeeded();
    bool _checkObsCollision  (const Obstacle&    o) const;
    bool _checkPteroCollision(const Pterodactyl& p) const;
    void _drawCloud(U8G2& disp, int x, int y)       const;
};