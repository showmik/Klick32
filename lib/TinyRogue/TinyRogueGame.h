#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include "ParticleManager.h"
#include "SceneGame.h"
#include "../Core/ECS.h"

enum class TurnAction : uint8_t {
    NONE = 0,
    COMPLETED = 1,
    OPEN_ALTAR = 2,
    OPEN_MERCHANT = 3,
    DESCEND_STAIRS = 4,
    GAME_OVER = 5
};

// ─── Game Data Structures ───────────────────────────────────────────────────

enum class ItemType : uint8_t { 
    NONE, POTION, ELIXIR, SCROLL_UPGRADE, THROWING_DART,
    DAGGER, SWORD, AXE, 
    LEATHER, CHAINMAIL, PLATE,
    RING_VAMPIRE, RING_WEALTH, RING_OWL, RING_BERSERKER
};

struct Item {
    ItemType type = ItemType::NONE;
    uint8_t level = 0;
    uint8_t count = 0;
};

enum class TileType : uint8_t { 
    WALL, FLOOR, CORRIDOR, DOOR, STAIRS_DOWN, CHEST, MERCHANT, SPIKE, TALL_GRASS,
    LOCKED_DOOR, KEY, ALTAR, WATER, RUBBLE, WEB
};

enum class MonsterType : uint8_t { 
    RAT, BAT, GOBLIN, ORC, SKELETON, TROLL, BOSS 
};

enum class LevelMutator : uint8_t {
    NONE, PITCH_BLACK, INFESTED, TREASURE_TROVE, 
    FLOODED, OVERGROWN, LABYRINTH
};

enum class Biome : uint8_t { SEWERS, PRISON, DEEP_CAVES, BOSS_ARENA };

// --- ECS Components ---
struct CTransform { int x = 0, y = 0; };
struct CHealth { int hp = 10, maxHp = 10; };
struct CCombat { int baseAttack = 2, baseDefense = 0, attack = 2, defense = 0, dodge = 0, critChance = 10; };
struct CPlayer { int xp = 0, level = 1, rootDuration = 0; };
struct CMonster { MonsterType type; bool alert = false; int spawnTurn = 0; };

// ─── Shared Game State ───────────────────────────────────────────────────────
struct RogueSharedData {
    static constexpr int MAP_W = 32;
    static constexpr int MAP_H = 32;
    static constexpr int MAX_ENTITIES = 64; // Increased to 64 for future projectiles etc.

    TileType map[MAP_H][MAP_W];
    bool explored[MAP_H][MAP_W]; // Fog of War tracking

    ECSRegistry<MAX_ENTITIES> registry;
    ComponentPool<CTransform, MAX_ENTITIES> transforms;
    ComponentPool<CHealth, MAX_ENTITIES> healths;
    ComponentPool<CCombat, MAX_ENTITIES> combats;
    ComponentPool<CPlayer, MAX_ENTITIES> players;
    ComponentPool<CMonster, MAX_ENTITIES> monsters;

    EntityID playerID;
    
    uint32_t currentDepth = 1;
    Biome currentBiome = Biome::SEWERS; // NEW
    uint32_t gold = 0;
    uint32_t hiScore = 0;
    uint32_t turnCount = 0;
    uint8_t keys = 0;
    bool inventoryTurnUsed = false;
    LevelMutator currentMutator = LevelMutator::NONE;
    
    static constexpr int MAX_INVENTORY = 6;
    Item inventory[MAX_INVENTORY];
    
    Item equippedWeapon;
    Item equippedArmor;
    Item equippedAccessory;
    char hudMessage[32] = "";
    uint8_t hudMessageTimer = 0;
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
class RogueInventoryScene;

class RogueInventoryScene : public Scene {
public:
    void setData(RogueSharedData* d) { _data = d; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    RogueSharedData* _data = nullptr;
    uint8_t _cursor = 0;
    char _msg[32] = "";
    uint8_t _msgTimer = 0;
    bool _upgrading = false;
    uint8_t _upgradeSelect = 0;
    bool _itemMenuOpen = false;
    uint8_t _itemMenuCursor = 0;

    void _cleanInventory();
};

class RogueTitleScene : public Scene {
public:
    void setData(RogueSharedData* d) { _data = d; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    uint32_t _frame = 0;
    RogueSharedData* _data = nullptr;
    
    struct Bat { float x, y, vx, vy; };
    Bat _bats[5];
    struct Spark { float x, y, vy; int life; };
    Spark _sparks[15];
};

class RogueShopScene : public Scene {
public:
    void setData(RogueSharedData* d)     { _data = d; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    RogueSharedData* _data = nullptr;
    uint8_t          _cursor = 0;
    char             _msg[32] = "";
    uint8_t          _msgTimer = 0;
    uint8_t          _introFrames = 0;
};

class RoguePlayScene : public Scene {
public:
    void setData      (RogueSharedData* d) { _data = d; }
    void setEngine    (Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void resumeSavedGame() { _resumed = true; }
    
    void saveSnapshot(Console& ctx) override;
    void loadSnapshot(Console& ctx) override;
    void onSnapshotRestored(Console& ctx) override;
    
    // Public state for external triggering (Aim Mode)
    bool isAiming = false;
    int aimX = 0, aimY = 0;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

    void drawDungeon(Console& ctx, int ox = 0, int oy = 0) const;

private:
    RogueSharedData* _data   = nullptr;
    Camera*          _camera = nullptr;
    ParticleManager* _particles = nullptr;
    
    bool _resumed = false;
    
    // Fade Transition State
    bool _descending = false;
    int8_t _fadeTimer = 0; 
    
    // Altar State
    bool _altarMenuOpen = false;
    uint8_t _altarMenuCursor = 0;
    int _activeAltarX = 0;
    int _activeAltarY = 0;
    

              // Shared monster spawner

    
    void _updateCamera(bool snap = false);
};

class RoguePauseScene : public Scene {
public:
    void onEnter(Console& ctx) override { _introFrames = 0; } // Reset on enter
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    uint8_t _introFrames = 0; // Track animation state
};

class RogueDeadScene : public Scene {
public:
    void setData     (RogueSharedData* d) { _data = d; }
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    RogueSharedData* _data = nullptr;
    uint8_t          _frame = 0;
};

// ─── Main Game Class ─────────────────────────────────────────────────────────

class TinyRogueGame : public SceneGame<RogueSharedData> {
public:
    void reset() override {
        TinyRogueGame* defaultState = new TinyRogueGame();
        *this = *defaultState;
        delete defaultState;
    }
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    
    const char*    getName()   const override;
    const uint8_t* getCoverArt() const override;

private:
    RogueTitleScene _title;
    RoguePlayScene  _play;
    RoguePauseScene _pause;
    RogueDeadScene  _dead;
    RogueShopScene  _shop;
    RogueInventoryScene _inventory;
};
