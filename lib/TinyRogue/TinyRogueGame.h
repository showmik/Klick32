#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"

// ─── Game Data Structures ───────────────────────────────────────────────────

enum class TileType : uint8_t { 
    WALL, FLOOR, CORRIDOR, DOOR, STAIRS_DOWN, CHEST, MERCHANT 
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
    int defense = 0;
    int level = 1;
    int xp = 0;
};

struct Monster : public Entity {
    MonsterType type;
    bool active = false;
};

// ─── Shared Game State ───────────────────────────────────────────────────────
struct RogueSharedData {
    static constexpr int MAP_W = 32;
    static constexpr int MAP_H = 32;
    static constexpr int MAX_MONSTERS = 15;

    TileType map[MAP_H][MAP_W];
    bool explored[MAP_H][MAP_W]; // Fog of War tracking
    Entity player;
    Monster monsters[MAX_MONSTERS];
    
    uint32_t currentDepth = 1;
    uint32_t gold = 0;
    uint32_t hiScore = 0;
    uint32_t turnCount = 0;
};

struct BSPNode {
    Rect bounds;
    Rect room;
    int leftNode = -1;
    int rightNode = -1;
};

// ─── Scene Declarations ──────────────────────────────────────────────────────

class RoguePlayScene;
class RoguePauseScene;
class RogueDeadScene;
class RogueShopScene;

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

class RogueShopScene : public Scene {
public:
    void setPlayScene(RoguePlayScene* p) { _play = p; }
    void setData(RogueSharedData* d)     { _data = d; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    RoguePlayScene*  _play = nullptr;
    RogueSharedData* _data = nullptr;
    uint8_t          _cursor = 0;
    char             _msg[32] = "";
    uint8_t          _msgTimer = 0;
    uint8_t          _introFrames = 0;
};

class RoguePlayScene : public Scene {
public:
    void setData      (RogueSharedData* d) { _data = d; }
    void setPauseScene(RoguePauseScene* p) { _pause = p; }
    void setDeadScene (RogueDeadScene* d)  { _dead = d; }
    void setShopScene (RogueShopScene* s)  { _shop = s; }

    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;

    void drawDungeon(Console& ctx, int ox = 0, int oy = 0) const;

private:
    RogueSharedData* _data   = nullptr;
    RoguePauseScene* _pause  = nullptr;
    RogueDeadScene*  _dead   = nullptr;
    RogueShopScene*  _shop   = nullptr;
    char _hudMessage[32] = "";
    uint8_t _hudMessageTimer = 0;

    int _camPixelX  = 0;
    int _camPixelY  = 0;
    int _camStartX  = 0;
    int _camStartY  = 0;
    int _camTargetX = 0;
    int _camTargetY = 0;
    int _camT       = 0;
    
    static constexpr int CAM_FRAMES = 6; 
    uint8_t _shakeFrames = 0; 

    Monster* _getMonsterAt(int x, int y) const;
    void _processMonsterTurns(Console& ctx, SceneManager& sm);

    void _generateMap();            // The "Router"
    void _generateBSPMap();         // The Castle generator
    void _generateCaveMap();        // The Cave generator
    void _spawnMonsters();          // Shared monster spawner

    void _processTurn(Console& ctx, SceneManager& sm, int dx, int dy);
    void _updateCamera(bool snap = false);

    // Combat Particles
    struct BloodSpurt { float x, y, vx, vy; uint8_t life; };
    static constexpr uint8_t MAX_BLOOD = 10;
    BloodSpurt _blood[MAX_BLOOD] = {};

    void _spawnHitEffect(int gridX, int gridY);
};

class RoguePauseScene : public Scene {
public:
    void setPlayScene(RoguePlayScene* p) { _play = p; }
    void onEnter(Console& ctx) override { _introFrames = 0; } // Reset on enter
    void update (Console& ctx, SceneManager& sm) override;
    void draw   (Console& ctx) override;
private:
    RoguePlayScene* _play = nullptr;
    uint8_t _introFrames = 0; // Track animation state
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
    RogueShopScene  _shop;
};