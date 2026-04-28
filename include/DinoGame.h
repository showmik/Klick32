#pragma once
#include "GameBase.h"

// ─── DinoGame ─────────────────────────────────────────────────────────────────
// Chrome-style endless runner. Sprites are 16×16 px.
//
// Controls (in-game):
//   UP / A    → Jump
//   MENU1 / B → Return to OS menu
class DinoGame : public GameBase {
public:
    // ── GameBase interface ────────────────────────────────────────────────────
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
        float x;
        bool  active;
        bool  large; // true = 12 px wide, false = 8 px wide
    };

    // ── Game constants ────────────────────────────────────────────────────────
    static constexpr int     GROUND_Y   = 52;
    static constexpr int     DINO_X     = 10;
    static constexpr int     DINO_W     = 16;
    static constexpr int     DINO_H     = 16;
    static constexpr int     CACTUS_H   = 16;
    static constexpr int     SMALL_W    = 8;
    static constexpr int     LARGE_W    = 12;
    static constexpr uint8_t MAX_OBS    = 2;

    static constexpr float   GRAVITY    = 0.55f;
    static constexpr float   JUMP_VY    = -8.0f;
    static constexpr float   INIT_SPEED = 2.5f;
    static constexpr float   MAX_SPEED  = 7.5f;
    static constexpr float   SPEED_INC  = 0.002f;

    static constexpr int     MIN_GAP    = 55;
    static constexpr int     MAX_GAP    = 120;

    // ── Member variables ──────────────────────────────────────────────────────
    DinoState _state    = DinoState::RUNNING;
    bool      _running  = false;

    float     _dinoY    = 0.0f;
    float     _dinoVY   = 0.0f;
    bool      _onGround = true;

    uint32_t  _score    = 0;
    uint32_t  _hiScore  = 0; // persists across restarts until power-off
    float     _speed    = INIT_SPEED;

    Obstacle  _obs[MAX_OBS] = {};
    uint32_t  _frameCnt     = 0;
    uint32_t  _animTimer    = 0;
    uint8_t   _animFrame    = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();
    void _spawnIfNeeded();
    bool _checkCollision(const Obstacle& o) const;
};