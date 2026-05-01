#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"

// ─── Game Data Structures ───────────────────────────────────────────────────

enum class TileType : uint8_t { 
    WALL, FLOOR, CORRIDOR, DOOR, STAIRS_DOWN, CHEST 
};

enum class MonsterType : uint8_t { 
    RAT, BAT, GOBLIN, ORC, SKELETON, TROLL 
};

struct Entity {
    int x = 0;
    int y = 0;
    int hp = 10;
    int maxHp = 10;
    int attack = 2;
    // --- New Progression Stats ---
    int level = 1;
    int xp = 0;
};

// Inside RogueSharedData, we already have gold and currentDepth.

struct Monster : public Entity {
    MonsterType type;
    bool active = false;
};

// ─── Shared Game State ───────────────────────────────────────────────────────
// Owned by TinyRogueGame. All scenes receive a pointer to this.
struct RogueSharedData {
    // Keep map sizes reasonable for memory and generation speed
    static constexpr int MAP_W = 32;
    static constexpr int MAP_H = 32;
    static constexpr int MAX_MONSTERS = 15;

    TileType map[MAP_H][MAP_W];
    Entity player;
    Monster monsters[MAX_MONSTERS];
    
    uint32_t currentDepth = 1;
    uint32_t gold = 0;
    uint32_t hiScore = 0;
};

// ─── Scene Declarations ──────────────────────────────────────────────────────

class RoguePlayScene;
class RoguePauseScene;
class RogueDeadScene;

class RogueTitleScene : public Scene {
public:
    void setPlayScene(RoguePlayScene* p) { _play = p; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    RoguePlayScene* _play = nullptr;
    uint8_t         _frame = 0;
};

class RoguePlayScene : public Scene {
public:
    void setData      (RogueSharedData* d) { _data = d; }
    void setPauseScene(RoguePauseScene* p) { _pause = p; }
    void setDeadScene (RogueDeadScene* d)  { _dead = d; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;

    // Helper exposed so PauseScene can draw the dungeon in the background
    void drawDungeon(Console& ctx, int ox = 0, int oy = 0) const;

private:
    RogueSharedData* _data   = nullptr;
    RoguePauseScene* _pause  = nullptr;
    RogueDeadScene*  _dead   = nullptr;

    // Camera offset
    int _camX = 0;
    int _camY = 0;

    uint8_t _shakeFrames = 0; // Adds some "juice" when the player takes damage

    // New AI & Combat Helpers
    Monster* _getMonsterAt(int x, int y) const;
    void _processMonsterTurns(Console& ctx, SceneManager& sm);

    void _generateMap();
    void _processTurn(int dx, int dy);
    void _updateCamera();
};

class RoguePauseScene : public Scene {
public:
    void setPlayScene(RoguePlayScene* p) { _play = p; }
    void onEnter(Console& ctx) override {}
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    RoguePlayScene* _play = nullptr;
};

class RogueDeadScene : public Scene {
public:
    void setData     (RogueSharedData* d) { _data = d; }
    void setPlayScene(RoguePlayScene* p)  { _play = p; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    RogueSharedData* _data = nullptr;
    RoguePlayScene*  _play = nullptr;
    uint8_t          _frame = 0;
};

// ─── Main Game Class ─────────────────────────────────────────────────────────

class TinyRogueGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    void update (Console& ctx) override;
    void draw   (Console& ctx) override;
    
    bool           isRunning() const override;
    const char*    getName()   const override;

private:
    RogueSharedData _data;
    SceneManager    _sm;
    RogueTitleScene _title;
    RoguePlayScene  _play;
    RoguePauseScene _pause;
    RogueDeadScene  _dead;
};