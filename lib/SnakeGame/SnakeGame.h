#pragma once
#include "GameBase.h"
#include "GameUtils.h"

class SnakeGame : public GameBase {
public:
    void onEnter(Console& ctx) override; // <--- Added Console& ctx
    void onExit(Console& ctx)  override; // <--- Added Console& ctx
    void update(Console& ctx) override;
    void draw(Console& ctx)   override;
    bool           isRunning() const override;
    const char*    getName()   const override;

private:
    enum class State { PLAYING, DEAD };
    enum class Dir   { UP, DOWN, LEFT, RIGHT };

    // ── Grid Constants ────────────────────────────────────────────────────────
    static constexpr int BLOCK_SIZE  = 4;
    static constexpr int GRID_W      = 32; // 128 / 4 = 32 columns
    static constexpr int GRID_H      = 14; // 56 / 4 = 14 rows
    static constexpr int TOP_OFFSET  = 8;  // Leave top 8 pixels for score
    static constexpr int MAX_SNAKE   = 128;
    
    // Starting speed (frames per grid move). Lower is faster.
    static constexpr uint8_t START_SPEED = 5; 

    // ── Members ───────────────────────────────────────────────────────────────
    State    _state      = State::PLAYING;
    bool     _running    = false;
    
    Dir      _dir        = Dir::RIGHT;
    Dir      _nextDir    = Dir::RIGHT;
    
    int      _len        = 0;
    int      _sx[MAX_SNAKE] = {};
    int      _sy[MAX_SNAKE] = {};
    
    int      _ax         = 0;
    int      _ay         = 0;
    
    uint32_t _score      = 0;
    uint32_t _hiScore    = 0;
    
    uint8_t  _moveTimer  = 0;
    uint8_t  _speed      = START_SPEED;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();
    void _spawnApple();
};