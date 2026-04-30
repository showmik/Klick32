#pragma once
#include "GameBase.h"
#include "GameUtils.h"

// ─── DinoGame ─────────────────────────────────────────────────────────────────
// Chrome-style endless runner.
//
// Controls:
//   UP  / A   → Jump
//   DOWN / B  → Duck  (ground only; suppresses jump)
//   MENU1     → Return to OS menu
// ─────────────────────────────────────────────────────────────────────────────
class DinoGame : public GameBase {
public:
    void onEnter() override;
    void onExit()  override;
    void update(Console& ctx) override;
    void draw(Console& ctx)   override;
    bool           isRunning() const override;
    const char*    getName()   const override;
    const uint8_t* getIcon()   const override;

private:
    enum class DinoState { RUNNING, DEAD };

    enum class ObstacleKind : uint8_t {
        CACTUS_SMALL,
        CACTUS_LARGE,
        PTERO_LOW,
        PTERO_HIGH
    };

    struct Obstacle {
        float        x;
        bool         active;
        ObstacleKind kind;
        uint8_t      animFrame;
        uint8_t      animTimer;
    };

    struct Cloud { Vec2 pos; };

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
    static constexpr float    GRAVITY            = 0.55f;
    static constexpr float    JUMP_VY            = -8.0f;
    static constexpr float    INIT_SPEED         = 2.5f;
    static constexpr float    MAX_SPEED          = 7.5f;
    static constexpr float    SPEED_INC          = 0.002f;
    static constexpr int      MIN_GAP            = 55;
    static constexpr int      MAX_GAP            = 120;
    static constexpr uint32_t SCORE_MILESTONE    = 100;
    static constexpr uint8_t  FLASH_FRAMES       = 12;
    static constexpr uint8_t  COYOTE_FRAMES      = 6;
    static constexpr uint8_t  JUMP_BUFFER_FRAMES = 8;

    // ── Members ───────────────────────────────────────────────────────────────
    DinoState  _state         = DinoState::RUNNING;
    bool       _running       = false;
    bool       _isDucking     = false;
    float      _dinoY         = 0.0f;
    float      _dinoVY        = 0.0f;
    bool       _onGround      = true;
    uint8_t    _coyoteFrames  = 0;
    uint8_t    _jumpBuffer    = 0;
    uint32_t   _score         = 0;
    uint32_t   _hiScore       = 0;
    uint32_t   _lastMilestone = 0;
    uint8_t    _flashTimer    = 0;
    float      _speed         = INIT_SPEED;
    Obstacle   _obs[MAX_OBS]       = {};
    Cloud      _clouds[MAX_CLOUDS] = {};
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