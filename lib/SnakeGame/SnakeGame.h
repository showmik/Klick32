#pragma once
#include "GameBase.h"
#include "GameUtils.h"

class SnakeGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit(Console& ctx)  override;
    void update(Console& ctx) override;
    void draw(Console& ctx)   override;
    bool           isRunning() const override;
    const char*    getName()   const override;

private:
    enum class State { PLAYING, PAUSED, NAME_ENTRY, DEAD };
    enum class Dir   { UP, DOWN, LEFT, RIGHT };

    // ── Grid Constants ────────────────────────────────────────────────────────
    static constexpr int BLOCK_SIZE     = 4;
    static constexpr int GRID_W         = 32; 
    static constexpr int GRID_H         = 14; 
    static constexpr int TOP_OFFSET     = 8;  
    static constexpr int MAX_SNAKE      = 128;
    static constexpr int MAX_WALLS      = 20;
    static constexpr uint8_t START_SPEED = 6; 
    
    // Feature Constants
    static constexpr uint32_t BONUS_POINTS   = 50;
    static constexpr uint16_t BONUS_DURATION = 40;
    static constexpr uint16_t POISON_DURATION = 50;

    // ── Members ───────────────────────────────────────────────────────────────
    State    _state      = State::PLAYING;
    bool     _running    = false;
    
    Dir      _dir        = Dir::RIGHT;
    Dir      _inputQueue[2] = {};
    uint8_t  _queueLen   = 0;
    
    int      _len        = 0;
    int      _sx[MAX_SNAKE] = {};
    int      _sy[MAX_SNAKE] = {};
    
    int      _wx[MAX_WALLS] = {};
    int      _wy[MAX_WALLS] = {};
    int      _numWalls   = 0;
    
    int      _ax = 0, _ay = 0; // Normal Apple
    
    int      _bx = 0, _by = 0; // Bonus Apple
    bool     _bonusActive = false;
    uint16_t _bonusTimer  = 0;
    
    int      _px = 0, _py = 0; // Poison Food
    bool     _poisonActive = false;
    uint16_t _poisonTimer  = 0;
    
    // Score & Name Entry
    uint32_t _score      = 0;
    uint32_t _hiScore    = 0;
    bool     _newHiScore = false;
    char     _hiName[4]  = "AAA";
    char     _currName[4]= "AAA";
    uint8_t  _nameIdx    = 0;
    
    uint8_t  _moveTimer  = 0;
    uint8_t  _speed      = START_SPEED;
    uint8_t  _shakeFrames = 0;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void _initRound();
    bool _isOccupied(int x, int y) const;
    void _spawnApple();
    void _spawnBonus();
    void _spawnPoison();
    void _spawnWall();
    void _pushInput(Dir d);
    void _saveHighScore();
};