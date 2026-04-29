#pragma once
#include "GameBase.h"

// ─── DinoGame ─────────────────────────────────────────────────────────────────
// Chrome-style endless runner.
//
// Controls (in-game):
//   UP  / A   → Jump
//   DOWN / B  → Duck  (ground only; suppresses jump input)
//   MENU1     → Return to OS menu
//
// ── Obstacle pipeline ─────────────────────────────────────────────────────────
// All obstacles share one array.  When the spawn gate opens, a type is chosen:
//   CACTUS_SMALL / CACTUS_LARGE   always available
//   PTERO_LOW                     unlocked at PTERO_MIN_SCORE — must jump over
//   PTERO_HIGH                    unlocked at PTERO_MIN_SCORE — must duck under
//
// Pterodactyls never appear back-to-back; enforced in _spawnObsIfNeeded().
//
// ── Jump feel ─────────────────────────────────────────────────────────────────
// Coyote time : jump still fires for COYOTE_FRAMES after leaving the ground.
// Jump buffer : a press is remembered for JUMP_BUFFER_FRAMES before landing.
//
// ── Score flash ───────────────────────────────────────────────────────────────
// Every SCORE_MILESTONE points the score area inverts for FLASH_FRAMES frames
// and a short beep plays.
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
    // ── State machine ─────────────────────────────────────────────────────────
    enum class DinoState { RUNNING, DEAD };

    // ── Obstacle kinds ────────────────────────────────────────────────────────
    enum class ObstacleKind : uint8_t {
        CACTUS_SMALL,
        CACTUS_LARGE,
        PTERO_LOW,    // low flier  — player must jump
        PTERO_HIGH    // high flier — player must duck
    };

    struct Obstacle {
        float        x;
        bool         active;
        ObstacleKind kind;
        uint8_t      animFrame;   // pterodactyl wing frame (0/1)
        uint8_t      animTimer;   // counts up; resets at PTERO_ANIM_RATE
    };

    struct Cloud { float x; int8_t y; };

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

    // Pterodactyl sprite dimensions
    static constexpr int      PTERO_W            = 16;
    static constexpr int      PTERO_H            = 8;

    // Pterodactyl Y positions (top of sprite).
    // Verified against dino hitboxes (GROUND_Y=52, DINO_H=16, DUCK_H=8):
    //
    //   PTERO_LOW_Y = 36  →  sprite rows 36–44,  hitbox rows 37–43
    //     Standing dino hitbox  rows 38–50  → overlap  → must jump         ✓
    //     Ducking  dino hitbox  rows 45–50  → no overlap                   ✓
    //
    //   PTERO_HIGH_Y = 18  →  sprite rows 18–26,  hitbox rows 19–25
    //     Standing dino hitbox  rows 38–50  → no overlap → safe upright    ✓
    //     Ducking  dino hitbox  rows 45–50  → no overlap → safe ducking    ✓
    //     Mid-arc  dino (dinoY≈16, hitbox rows 18–30) → overlap            ✓
    //       Jumping into PTERO_HIGH = death; player must duck and hold.
    static constexpr int      PTERO_LOW_Y        = 36;
    static constexpr int      PTERO_HIGH_Y       = 18;

    // Frames between pterodactyl wing-flap toggles (~267 ms at 30 fps)
    static constexpr uint8_t  PTERO_ANIM_RATE    = 8;

    // Score at which pterodactyls unlock
    static constexpr uint32_t PTERO_MIN_SCORE    = 200;

    // Spawn weights: random(PTERO_W + CACTUS_W) < PTERO_W  →  pterodactyl
    // Gives 25 % pterodactyls once unlocked (never back-to-back).
    static constexpr uint8_t  PTERO_W_WEIGHT     = 1;
    static constexpr uint8_t  CACTUS_W_WEIGHT    = 3;

    static constexpr uint8_t  MAX_CLOUDS         = 3;

    static constexpr float    GRAVITY            = 0.55f;
    static constexpr float    JUMP_VY            = -8.0f;
    static constexpr float    INIT_SPEED         = 2.5f;
    static constexpr float    MAX_SPEED          = 7.5f;
    static constexpr float    SPEED_INC          = 0.002f;

    // Gap = pixels from right edge of previous obstacle to left edge of new one.
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
    uint8_t    _animTimer = 0;   // dino leg animation
    uint8_t    _animFrame = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();

    // Single spawn function handles all obstacle types.
    void _spawnObsIfNeeded();

    // Single collision function handles all obstacle types.
    bool _checkCollision(const Obstacle& o) const;

    // Per-kind queries — static, no instance state required.
    static int  _obsWidth(ObstacleKind k);  // pixel width
    static int  _obsTopY (ObstacleKind k);  // top pixel row when drawn
    static bool _isPtero (ObstacleKind k);  // true for PTERO_LOW / PTERO_HIGH

    void _drawCloud(U8G2& disp, int x, int y) const;
};