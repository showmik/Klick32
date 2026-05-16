#pragma once
#include "GameBase.h"
#include "GameUtils.h"
#include "SceneManager.h"
#include "Scene.h"
#include "Camera.h"
#include "AnimationManager.h"

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
    LOCKED_DOOR, KEY, ALTAR
};

enum class MonsterType : uint8_t { 
    RAT, BAT, GOBLIN, ORC, SKELETON, TROLL, BOSS 
};

enum class LevelMutator : uint8_t {
    NONE, PITCH_BLACK, INFESTED, TREASURE_TROVE
};

struct Entity {
    int x = 0;
    int y = 0;
    int hp = 10;
    int maxHp = 10;
    int baseAttack = 2;
    int baseDefense = 0;
    int attack = 2;
    int defense = 0;
    int level = 1;
    int xp = 0;
    int dodge = 0;      // Percentage (0-100)
    int critChance = 10; // Percentage (0-100)
};

struct Monster : public Entity {
    MonsterType type;
    bool active = false;
    bool alert = false;
};

// ─── Shared Game State ───────────────────────────────────────────────────────
struct RogueSharedData {
    static constexpr int MAP_W = 32;
    static constexpr int MAP_H = 32;
    static constexpr int MAX_MONSTERS = 25; // Increased to support Infested mutator

    TileType map[MAP_H][MAP_W];
    bool explored[MAP_H][MAP_W]; // Fog of War tracking
    Entity player;
    Monster monsters[MAX_MONSTERS];
    
    uint32_t currentDepth = 1;
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
    void onEnter(Console& ctx) override;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;
private:
    uint8_t _frame = 0;
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
    void setEngine    (Camera* cam, AnimationManager* anim) { _camera = cam; _particles = anim; }

    void onEnter(Console& ctx) override;
    void resumeSavedGame() { _resumed = true; }
    
    // Public state for external triggering (Aim Mode)
    bool isAiming = false;
    int aimX = 0, aimY = 0;
    void update (Console& ctx, SceneManager& sm, float dt) override;
    void draw   (Console& ctx) override;

    void drawDungeon(Console& ctx, int ox = 0, int oy = 0) const;

private:
    RogueSharedData* _data   = nullptr;
    Camera*          _camera = nullptr;
    AnimationManager* _particles = nullptr;
    char _hudMessage[32] = "";
    uint8_t _hudMessageTimer = 0;
    bool _resumed = false;
    
    // Fade Transition State
    bool _descending = false;
    int8_t _fadeTimer = 0; 
    
    // Altar State
    bool _altarMenuOpen = false;
    uint8_t _altarMenuCursor = 0;
    int _activeAltarX = 0;
    int _activeAltarY = 0;
    Monster* _getMonsterAt(int x, int y) const;
    void _processMonsterTurns(Console& ctx, SceneManager& sm);

    void _generateMap();            // The "Router"
    void _generateBSPMap();         // The Castle generator
    void _generateCaveMap();        // The Cave generator
    void _generateBossMap();        // The Boss Arena generator
    void _spawnMonsters();          // Shared monster spawner

    bool _processTurn(Console& ctx, SceneManager& sm, int dx, int dy);
    void _updateCamera(bool snap = false);

    void _spawnHitEffect(int gridX, int gridY);
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

class TinyRogueGame : public GameBase {
public:
    void onEnter(Console& ctx) override;
    void onExit (Console& ctx) override;
    void update (Console& ctx, float dt) override;
    void draw   (Console& ctx) override;
    
    bool           isRunning() const override;
    const char*    getName()   const override;
    const uint8_t* getCoverArt() const override;

private:
    RogueSharedData _data;
    SceneManager    _sm;
    Camera          _camera;
    AnimationManager _particles;
    RogueTitleScene _title;
    RoguePlayScene  _play;
    RoguePauseScene _pause;
    RogueDeadScene  _dead;
    RogueShopScene  _shop;
    RogueInventoryScene _inventory;
};
