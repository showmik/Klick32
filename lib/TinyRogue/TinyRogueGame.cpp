#include "TinyRogueGame.h"
#include "GameRegistry.h"
#include "TinyRogueSprites.h"

static int getWeaponAttack(ItemType t) {
    if (t == ItemType::DAGGER) return 1;
    if (t == ItemType::SWORD) return 2;
    if (t == ItemType::AXE) return 3;
    return 0;
}

static int getArmorDefense(ItemType t) {
    if (t == ItemType::LEATHER) return 1;
    if (t == ItemType::CHAINMAIL) return 2;
    if (t == ItemType::PLATE) return 3;
    return 0;
}

static const char* getItemName(ItemType t) {
    switch(t) {
        case ItemType::POTION: return "Potion";
        case ItemType::ELIXIR: return "Elixir";
        case ItemType::SCROLL_UPGRADE: return "Upg Scroll";
        case ItemType::THROWING_DART: return "Dart";
        case ItemType::DAGGER: return "Dagger";
        case ItemType::SWORD: return "Sword";
        case ItemType::AXE: return "Axe";
        case ItemType::LEATHER: return "Leather";
        case ItemType::CHAINMAIL: return "Chainmail";
        case ItemType::PLATE: return "Plate";
        default: return "-";
    }
}

static void recalcStats(RogueSharedData* _data) {
    _data->player.attack = _data->player.baseAttack + getWeaponAttack(_data->equippedWeapon.type) + _data->equippedWeapon.level;
    _data->player.defense = _data->player.baseDefense + getArmorDefense(_data->equippedArmor.type) + _data->equippedArmor.level;
}

// ═════════════════════════════════════════════════════════════════════════════
// RogueTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void RogueTitleScene::onEnter(Console& ctx) {
    _frame = 0;
}

void RogueTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Map CUSTOM_1 to PlayScene in Game
    }
}

void RogueTitleScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(32, 24, "TINY ROGUE");
    ctx.drawHLine(0, 30, Console::W);

    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        int w = ctx.strWidth("Press A to descend");
        ctx.drawStr((Console::W - w) / 2, 54, "Press A to descend");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RoguePlayScene
// ═════════════════════════════════════════════════════════════════════════════

void RoguePlayScene::onEnter(Console& ctx) {
    _descending = false;
    _fadeTimer = 0;
    if (_resumed) {
        _resumed = false;
        recalcStats(_data);
        _updateCamera(true);
        isAiming = false;
        return;
    }

    _data->currentDepth = 1;
    _data->gold = 0;
    _data->player.level = 1;
    _data->player.xp = 0;
    _data->player.maxHp = 10;
    _data->player.hp = 10;
    _data->player.baseAttack = 2;
    _data->player.baseDefense = 0;
    _data->equippedWeapon.type = ItemType::NONE;
    _data->equippedArmor.type = ItemType::NONE;
    recalcStats(_data);
    for (int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
        _data->inventory[i].type = ItemType::NONE;
    }
    _generateMap();
}

void RoguePlayScene::_spawnHitEffect(int gridX, int gridY) {
    int px = (gridX * 8) + 4;
    int py = (gridY * 8) + 4;
    for (int i = 0; i < 4; i++) {
        _particles->spawnPixel(px, py, (random(-20, 21) / 10.0f), (random(-20, 21) / 10.0f), random(5, 12));
    }
}

void RoguePlayScene::_generateMap() {
    // 1. Reset Fog of War
    memset(_data->explored, 0, sizeof(_data->explored));

    // 2. Biomes and Boss Arenas
    if (_data->currentDepth % 5 == 0) {
        _generateBossMap(); // Depths 5, 10, 15...
    } else if (_data->currentDepth < 5) {
        _generateCaveMap(); // Sewers
    } else if (_data->currentDepth < 10) {
        _generateBSPMap();  // Prison
    } else {
        if (random(100) < 50) _generateBSPMap(); else _generateCaveMap(); // Deep Caves
    }

    // 3. Shared Spawning & Setup
    _data->keys = 0;
    
    // Only spawn doors on non-boss levels
    if (_data->currentDepth % 5 != 0) {
        // Find stairs to ensure they are always reachable
        int sx = -1, sy = -1, totalFloors = 0;
        for (int y = 0; y < RogueSharedData::MAP_H; y++) {
            for (int x = 0; x < RogueSharedData::MAP_W; x++) {
                if (_data->map[y][x] == TileType::STAIRS_DOWN) { sx = x; sy = y; }
                if (_data->map[y][x] == TileType::FLOOR || _data->map[y][x] == TileType::CORRIDOR) totalFloors++;
            }
        }

        struct Coord { uint8_t x, y; };
        static Coord corridors[300];
        int corridorCount = 0;
        
        for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
            for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
                if (_data->map[y][x] == TileType::CORRIDOR) {
                    if (corridorCount < 300) corridors[corridorCount++] = {(uint8_t)x, (uint8_t)y};
                }
            }
        }

        // Shuffle corridors
        for (int i = 0; i < corridorCount; i++) {
            int j = random(corridorCount);
            Coord temp = corridors[i];
            corridors[i] = corridors[j];
            corridors[j] = temp;
        }

        static bool reachable[RogueSharedData::MAP_H][RogueSharedData::MAP_W];
        static Coord queue[1024];

        // Find a valid choke point
        for (int i = 0; i < corridorCount; i++) {
            Coord c = corridors[i];
            _data->map[c.y][c.x] = TileType::LOCKED_DOOR;
            
            memset(reachable, 0, sizeof(reachable));
            int head = 0, tail = 0;
            queue[tail++] = {(uint8_t)_data->player.x, (uint8_t)_data->player.y};
            reachable[_data->player.y][_data->player.x] = true;
            
            int reachableCount = 0;
            while(head < tail) {
                Coord curr = queue[head++];
                reachableCount++;
                
                int dx[] = {0, 0, -1, 1};
                int dy[] = {-1, 1, 0, 0};
                for(int d = 0; d < 4; d++) {
                    int nx = curr.x + dx[d];
                    int ny = curr.y + dy[d];
                    if (nx >= 0 && nx < RogueSharedData::MAP_W && ny >= 0 && ny < RogueSharedData::MAP_H) {
                        if (!reachable[ny][nx]) {
                            TileType t = _data->map[ny][nx];
                            if (t != TileType::WALL && t != TileType::LOCKED_DOOR) {
                                reachable[ny][nx] = true;
                                queue[tail++] = {(uint8_t)nx, (uint8_t)ny};
                            }
                        }
                    }
                }
            }
            
            // Check if valid bridge (Stairs are reachable, but some floors are blocked off)
            if (reachable[sy][sx] && reachableCount < totalFloors) {
                // 1. Place Key in reachable area
                Coord validKeys[400]; int validKeyCount = 0;
                for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
                    for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
                        if (reachable[y][x] && _data->map[y][x] == TileType::FLOOR && (x != _data->player.x || y != _data->player.y)) {
                            if (validKeyCount < 400) validKeys[validKeyCount++] = {(uint8_t)x, (uint8_t)y};
                        }
                    }
                }
                
                if (validKeyCount > 0) {
                    Coord kp = validKeys[random(validKeyCount)];
                    _data->map[kp.y][kp.x] = TileType::KEY;
                    
                    // 2. Place Chest in isolated area to guarantee reward
                    Coord validChests[400]; int validChestCount = 0;
                    for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
                        for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
                            if (!reachable[y][x] && _data->map[y][x] == TileType::FLOOR) {
                                if (validChestCount < 400) validChests[validChestCount++] = {(uint8_t)x, (uint8_t)y};
                            }
                        }
                    }
                    if (validChestCount > 0) {
                        Coord cp = validChests[random(validChestCount)];
                        _data->map[cp.y][cp.x] = TileType::CHEST;
                    }
                    break; // Successfully placed door!
                }
            }
            // Revert if not a valid choke point
            _data->map[c.y][c.x] = TileType::CORRIDOR;
        }
    }

    _spawnMonsters();
    _updateCamera(true); 
    isAiming = false;
}

void RoguePlayScene::_generateBSPMap() {
    for (int y = 0; y < RogueSharedData::MAP_H; y++) {
        for (int x = 0; x < RogueSharedData::MAP_W; x++) {
            _data->map[y][x] = TileType::WALL;
        }
    }

    const int MAX_NODES = 31;
    BSPNode nodes[MAX_NODES];
    int numNodes = 0;

    nodes[numNodes++] = { {1, 1, RogueSharedData::MAP_W - 2, RogueSharedData::MAP_H - 2}, {0,0,0,0}, -1, -1 };

    for (int i = 0; i < numNodes; i++) {
        if (numNodes >= MAX_NODES - 1) break;

        Rect b = nodes[i].bounds;
        bool splitH = random(2) == 0;
        if (b.w > b.h && b.w / b.h >= 1.25f) splitH = false; 
        else if (b.h > b.w && b.h / b.w >= 1.25f) splitH = true; 

        int maxSplit = (splitH ? b.h : b.w) - 6; 
        if (maxSplit <= 6) continue; 

        int splitLoc = random(6, maxSplit);

        nodes[i].leftNode = numNodes;
        nodes[i].rightNode = numNodes + 1;

        if (splitH) {
            nodes[numNodes++] = { {b.x, b.y, b.w, splitLoc}, {0,0,0,0}, -1, -1 };
            nodes[numNodes++] = { {b.x, b.y + splitLoc, b.w, b.h - splitLoc}, {0,0,0,0}, -1, -1 };
        } else {
            nodes[numNodes++] = { {b.x, b.y, splitLoc, b.h}, {0,0,0,0}, -1, -1 };
            nodes[numNodes++] = { {b.x + splitLoc, b.y, b.w - splitLoc, b.h}, {0,0,0,0}, -1, -1 };
        }
    }

    int leafCount = 0;
    int leafIndices[16];

    for (int i = 0; i < numNodes; i++) {
        if (nodes[i].leftNode == -1 && nodes[i].rightNode == -1) { 
            Rect b = nodes[i].bounds;
            int rw = random(4, b.w - 1);
            int rh = random(4, b.h - 1);
            int rx = b.x + random(1, b.w - rw);
            int ry = b.y + random(1, b.h - rh);

            nodes[i].room = {rx, ry, rw, rh};

            for (int y = ry; y < ry + rh; y++) {
                for (int x = rx; x < rx + rw; x++) {
                    _data->map[y][x] = TileType::FLOOR;
                }
            }
            if (leafCount < 16) leafIndices[leafCount++] = i;
        }
    }

    for (int i = numNodes - 1; i >= 0; i--) {
        if (nodes[i].leftNode != -1) {
            BSPNode& l = nodes[nodes[i].leftNode];
            BSPNode& r = nodes[nodes[i].rightNode];

            nodes[i].room = l.room; 
            int lx = l.room.x + l.room.w / 2;
            int ly = l.room.y + l.room.h / 2;
            int rx = r.room.x + r.room.w / 2;
            int ry = r.room.y + r.room.h / 2;

            int curX = lx, curY = ly;
            
            if (random(2) == 0) {
                while (curX != rx) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(rx - curX); }
                while (curY != ry) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(ry - curY); }
            } else {
                while (curY != ry) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(ry - curY); }
                while (curX != rx) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(rx - curX); }
            }
        }
    }

    if (leafCount > 0) {
        Rect firstRoom = nodes[leafIndices[0]].room;
        _data->player.x = firstRoom.x + firstRoom.w / 2;
        _data->player.y = firstRoom.y + firstRoom.h / 2;

        Rect lastRoom = nodes[leafIndices[leafCount - 1]].room;
        _data->map[lastRoom.y + lastRoom.h / 2][lastRoom.x + lastRoom.w / 2] = TileType::STAIRS_DOWN;
        
        for (int i = 1; i < leafCount - 1; i++) {
            if (random(100) < 30) {
                Rect r = nodes[leafIndices[i]].room;
                _data->map[r.y + 1][r.x + 1] = TileType::CHEST;
            }
        }
    }

    // Spawn exactly 1 Merchant on an open floor tile
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

    // Spawn Spikes
    int numSpikes = random(2, 7);
    for (int i = 0; i < numSpikes; i++) {
        int sx, sy;
        do {
            sx = random(1, RogueSharedData::MAP_W - 1);
            sy = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[sy][sx] != TileType::FLOOR || (sx == _data->player.x && sy == _data->player.y));
        _data->map[sy][sx] = TileType::SPIKE;
    }

    // Spawn Tall Grass
    for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
        for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
            if (_data->map[y][x] == TileType::FLOOR && random(100) < 15) {
                _data->map[y][x] = TileType::TALL_GRASS;
            }
        }
    }
}

void RoguePlayScene::_generateCaveMap() {
    // 1. Initial Noise (45% walls)
    for (int y = 0; y < RogueSharedData::MAP_H; y++) {
        for (int x = 0; x < RogueSharedData::MAP_W; x++) {
            if (x == 0 || x == RogueSharedData::MAP_W - 1 || y == 0 || y == RogueSharedData::MAP_H - 1) {
                _data->map[y][x] = TileType::WALL; // Hard borders
            } else {
                _data->map[y][x] = (random(100) < 45) ? TileType::WALL : TileType::FLOOR;
            }
        }
    }

    // 2. Cellular Automata Smoothing Passes
    static TileType temp[RogueSharedData::MAP_H][RogueSharedData::MAP_W];
    for (int i = 0; i < 4; i++) {
        for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
            for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
                int wallCount = 0;
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        if (dx == 0 && dy == 0) continue;
                        if (_data->map[y+dy][x+dx] == TileType::WALL) wallCount++;
                    }
                }
                
                // Rule: Become wall if crowded, become floor if open
                if (wallCount >= 5) temp[y][x] = TileType::WALL;
                else if (wallCount <= 3) temp[y][x] = TileType::FLOOR;
                else temp[y][x] = _data->map[y][x];
            }
        }
        // Copy back
        for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
            for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
                _data->map[y][x] = temp[y][x];
            }
        }
    }

    // 3. Helper to find open space
    auto getOpenTile = [&]() {
        int tx, ty;
        do {
            tx = random(1, RogueSharedData::MAP_W - 1);
            ty = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[ty][tx] != TileType::FLOOR);
        return Vec2{(float)tx, (float)ty};
    };

    // 4. Place Player, Stairs, and Loot
    Vec2 p = getOpenTile();
    _data->player.x = p.ix();
    _data->player.y = p.iy();

    Vec2 s;
    int attempts = 0; // Prevent infinite loop on tiny disconnected maps
    do { 
        s = getOpenTile(); 
        attempts++;
    } while (abs(s.ix() - _data->player.x) + abs(s.iy() - _data->player.y) < 15 && attempts < 50); 
    _data->map[s.iy()][s.ix()] = TileType::STAIRS_DOWN;

    // --- BUG FIX: Guarantee connectivity between player and stairs ---
    int curX = _data->player.x;
    int curY = _data->player.y;
    
    if (random(2) == 0) {
        while (curX != s.ix()) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(s.ix() - curX); }
        while (curY != s.iy()) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(s.iy() - curY); }
    } else {
        while (curY != s.iy()) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(s.iy() - curY); }
        while (curX != s.ix()) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(s.ix() - curX); }
    }
    // ---------------------------------------------------------------

    int numChests = random(1, 4);
    for (int i = 0; i < numChests; i++) {
        Vec2 c = getOpenTile();
        if (c.ix() != _data->player.x || c.iy() != _data->player.y) {
            _data->map[c.iy()][c.ix()] = TileType::CHEST;
        }
    }

    // Spawn exactly 1 Merchant on an open floor tile
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

    // Spawn Spikes
    int numSpikes = random(2, 7);
    for (int i = 0; i < numSpikes; i++) {
        int sx, sy;
        do {
            sx = random(1, RogueSharedData::MAP_W - 1);
            sy = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[sy][sx] != TileType::FLOOR || (sx == _data->player.x && sy == _data->player.y));
        _data->map[sy][sx] = TileType::SPIKE;
    }

    // Spawn Tall Grass
    for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
        for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
            if (_data->map[y][x] == TileType::FLOOR && random(100) < 15) {
                _data->map[y][x] = TileType::TALL_GRASS;
            }
        }
    }
}

void RoguePlayScene::_generateBossMap() {
    for (int y = 0; y < RogueSharedData::MAP_H; y++) {
        for (int x = 0; x < RogueSharedData::MAP_W; x++) {
            if (x >= 8 && x <= 24 && y >= 8 && y <= 24) {
                if (x == 8 || x == 24 || y == 8 || y == 24) _data->map[y][x] = TileType::WALL;
                else _data->map[y][x] = TileType::FLOOR;
            } else {
                _data->map[y][x] = TileType::WALL;
            }
        }
    }
    _data->player.x = 16;
    _data->player.y = 22;
}

void RoguePlayScene::_spawnMonsters() {
    for (auto& m : _data->monsters) m.active = false;

    if (_data->currentDepth % 5 == 0) {
        // Boss Room Setup
        _data->monsters[0].active = true;
        _data->monsters[0].type = MonsterType::BOSS;
        _data->monsters[0].x = 16;
        _data->monsters[0].y = 12;
        _data->monsters[0].maxHp = 25 + (_data->currentDepth * 5);
        _data->monsters[0].hp = _data->monsters[0].maxHp;
        _data->monsters[0].attack = 3 + (_data->currentDepth / 2);
        _data->monsters[0].alert = true;
        return;
    }

    int targetMonsters = (_data->currentDepth * 2) + 6;
    if (targetMonsters > RogueSharedData::MAX_MONSTERS) {
        targetMonsters = RogueSharedData::MAX_MONSTERS;
    }

    for (int i = 0; i < targetMonsters; i++) {
        int mx, my;
        bool validSpot = false;
        
        while (!validSpot) {
            mx = random(1, RogueSharedData::MAP_W - 1);
            my = random(1, RogueSharedData::MAP_H - 1);
            
            if (_data->map[my][mx] == TileType::FLOOR && (mx != _data->player.x || my != _data->player.y)) {
                bool occupied = false;
                for (int j = 0; j < i; j++) {
                    if (_data->monsters[j].x == mx && _data->monsters[j].y == my) {
                        occupied = true; break;
                    }
                }
                if (!occupied) validSpot = true;
            }
        }

        _data->monsters[i].x = mx;
        _data->monsters[i].y = my;
        _data->monsters[i].active = true;
        _data->monsters[i].hp = 4 + _data->currentDepth;
        _data->monsters[i].maxHp = _data->monsters[i].hp;
        _data->monsters[i].attack = 1 + (_data->currentDepth / 3);
        _data->monsters[i].alert = false;

        if (_data->currentDepth < 5) {
            // Sewers
            int r = random(3);
            if (r == 0) _data->monsters[i].type = MonsterType::RAT;
            else if (r == 1) _data->monsters[i].type = MonsterType::BAT;
            else _data->monsters[i].type = MonsterType::GOBLIN;
        } else if (_data->currentDepth < 10) {
            // Prison
            int r = random(3);
            if (r == 0) _data->monsters[i].type = MonsterType::SKELETON;
            else if (r == 1) _data->monsters[i].type = MonsterType::ORC;
            else _data->monsters[i].type = MonsterType::GOBLIN;
        } else {
            // Deep Caves
            int r = random(3);
            if (r == 0) _data->monsters[i].type = MonsterType::TROLL;
            else if (r == 1) _data->monsters[i].type = MonsterType::ORC;
            else _data->monsters[i].type = MonsterType::BAT;
            
            if (_data->monsters[i].type == MonsterType::TROLL) {
                _data->monsters[i].hp += 8;
                _data->monsters[i].attack += 2;
            }
        }
    }
}

void RoguePlayScene::_updateCamera(bool snap) {
    int focusX = isAiming ? aimX : _data->player.x;
    int focusY = isAiming ? aimY : _data->player.y;

    int targetX = (focusX * 8) - (Console::W / 2) + 4;
    int targetY = (focusY * 8) - (Console::H / 2) + 4;

    targetX = gclamp(targetX, 0, (RogueSharedData::MAP_W * 8) - Console::W);
    targetY = gclamp(targetY, 0, (RogueSharedData::MAP_H * 8) - Console::H);

    if (snap) {
        _camera->snapTo(targetX, targetY);
    } else {
        _camera->panTo(targetX, targetY, 6);
    }
}

void RoguePlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (_hudMessageTimer > 0) _hudMessageTimer--;
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }

    // --- Fade Transition Logic ---
    if (_descending) {
        _fadeTimer--;
        if (_fadeTimer == 0) {
            // Screen is completely black, generate the next level!
            _data->currentDepth++;
            _generateMap(); 
            _camera->snapTo((_data->player.x * 8) - (Console::W / 2) + 4, 
                            (_data->player.y * 8) - (Console::H / 2) + 4);
            _descending = false;
            _fadeTimer = -20; // Use negative numbers to track the fade-in opening animation
        }
        return; // Block all inputs while fading out
    }
    
    if (_fadeTimer < 0) {
        _fadeTimer++;
        if (_fadeTimer < 0) return; // Block all inputs while fading in
    }
    // -----------------------------

    if (_data->inventoryTurnUsed) {
        _data->inventoryTurnUsed = false;
        _processMonsterTurns(ctx, sm);
    }

    if (ctx.justPressed(Btn::MENU2)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::PAUSE);
        return;
    }
    if (ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::CUSTOM_3); // Open Inventory
        return;
    }

    if (isAiming) {
        if (ctx.justPressed(Btn::UP))    aimY = max(1, aimY - 1);
        if (ctx.justPressed(Btn::DOWN))  aimY = min(RogueSharedData::MAP_H - 2, aimY + 1);
        if (ctx.justPressed(Btn::LEFT))  aimX = max(1, aimX - 1);
        if (ctx.justPressed(Btn::RIGHT)) aimX = min(RogueSharedData::MAP_W - 2, aimX + 1);
        
        if (ctx.justPressed(Btn::A)) {
            // Check Line of Sight
            bool hitWall = false;
            int px = _data->player.x, py = _data->player.y;
            int steps = max(abs(aimX - px), abs(aimY - py));
            
            if (steps > 0) {
                for (int i = 0; i <= steps; i++) {
                    int lx = px + (aimX - px) * i / steps;
                    int ly = py + (aimY - py) * i / steps;
                    if (lx < 0 || lx >= RogueSharedData::MAP_W || ly < 0 || ly >= RogueSharedData::MAP_H ||
                        _data->map[ly][lx] == TileType::WALL || _data->map[ly][lx] == TileType::LOCKED_DOOR) {
                        hitWall = true; 
                        break;
                    }
                }
            }

            if (hitWall) {
                snprintf(_hudMessage, sizeof(_hudMessage), "Path Blocked!");
                _hudMessageTimer = 40;
                ctx.beep(150, 100);
            } else {
                // Consume dart
                for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                    if (_data->inventory[i].type == ItemType::THROWING_DART) {
                        _data->inventory[i].count--;
                        if (_data->inventory[i].count == 0) _data->inventory[i].type = ItemType::NONE;
                        break;
                    }
                }

                Monster* m = _getMonsterAt(aimX, aimY);
                if (m) {
                    m->hp -= 3;
                    _spawnHitEffect(aimX, aimY);
                    ctx.beep(1200, 30);
                    
                    if (m->hp <= 0) {
                        m->active = false;
                        _data->player.xp += m->maxHp;
                        
                        bool leveledUp = false;
                        while (_data->player.xp >= _data->player.level * 10) {
                            _data->player.xp -= _data->player.level * 10;
                            _data->player.level++;
                            _data->player.maxHp += 5;
                            _data->player.hp = _data->player.maxHp; 
                            _data->player.baseAttack += 1;
                            recalcStats(_data);
                            leveledUp = true;
                        }
                        if (leveledUp) {
                            snprintf(_hudMessage, sizeof(_hudMessage), "LEVEL UP!");
                            _hudMessageTimer = 60;
                            ctx.beep(800, 100); ctx.beep(1200, 150);
                        }
                        if (m->type == MonsterType::BOSS) {
                            _data->map[m->y][m->x] = TileType::STAIRS_DOWN;
                            if (_data->map[m->y + 1][m->x] == TileType::FLOOR) {
                                _data->map[m->y + 1][m->x] = TileType::CHEST;
                            }
                            snprintf(_hudMessage, sizeof(_hudMessage), "Boss Defeated!");
                            _hudMessageTimer = 80;
                            ctx.beep(1500, 200);
                        }
                    }
                } else {
                    // Missed / Threw at empty floor
                    ctx.sfxPoint(); 
                }
                
                isAiming = false;
                _processMonsterTurns(ctx, sm);
            }
        }
        else if (ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU2)) {
            isAiming = false;
            ctx.sfxMenuBack();
        }
        return;
    }

    int dx = 0, dy = 0;
    bool waited = false;
    if (ctx.justPressed(Btn::UP)   || ctx.repeat(Btn::UP))   dy = -1;
    else if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) dy = 1;
    else if (ctx.justPressed(Btn::LEFT) || ctx.repeat(Btn::LEFT)) dx = -1;
    else if (ctx.justPressed(Btn::RIGHT)|| ctx.repeat(Btn::RIGHT)) dx = 1;
    else if (ctx.justPressed(Btn::A)) waited = true;

    if (dx != 0 || dy != 0 || waited) {
        if (_processTurn(ctx, sm, dx, dy)) {
            _processMonsterTurns(ctx, sm); 
        }
    }

    _updateCamera(); 
}

Monster* RoguePlayScene::_getMonsterAt(int x, int y) const {
    for (auto& m : _data->monsters) {
        if (m.active && m.x == x && m.y == y) return &m;
    }
    return nullptr;
}

void RoguePlayScene::_processMonsterTurns(Console& ctx, SceneManager& sm) {
    bool playerHit = false;

    for (auto& m : _data->monsters) {
        if (!m.active || _data->player.hp <= 0) continue;

        int aggro = 6;
        if (m.type == MonsterType::BAT) aggro = 8;
        else if (m.type == MonsterType::SKELETON) aggro = 5;
        else if (m.type == MonsterType::BOSS) aggro = 15; // Boss tracks across the whole room

        // Tall grass hides the player, severely reducing monster sight radius
        if (_data->map[_data->player.y][_data->player.x] == TileType::TALL_GRASS) {
            aggro = 2; 
        }

        if (m.type == MonsterType::SKELETON && (_data->turnCount % 2 != 0)) {
            continue; 
        }
        if (m.type == MonsterType::TROLL && (_data->turnCount % 2 == 0)) {
            continue; // Trolls are slow
        }

        int steps = (m.type == MonsterType::BAT) ? 2 : 1;

        for (int s = 0; s < steps; s++) {
            int dx = _data->player.x - m.x;
            int dy = _data->player.y - m.y;

            if (abs(dx) <= aggro && abs(dy) <= aggro) {
                m.alert = true; // Monster woke up or spotted player
                int stepX = 0, stepY = 0;

                if (m.type == MonsterType::BAT && random(3) == 0) {
                    if (random(2) == 0) stepX = (random(2) == 0) ? 1 : -1;
                    else                stepY = (random(2) == 0) ? 1 : -1;
                } 
                else if (m.type == MonsterType::RAT && abs(dx) > 3 && abs(dy) > 3) {
                    if (random(2) == 0) stepX = (random(2) == 0) ? 1 : -1;
                    else                stepY = (random(2) == 0) ? 1 : -1;
                }
                else {
                    stepX = gsign(dx);
                    stepY = gsign(dy);

                    if (stepX != 0 && stepY != 0) {
                        if (random(2) == 0) stepY = 0; 
                        else stepX = 0;
                    }
                }

                int nx = m.x + stepX;
                int ny = m.y + stepY;

                if (nx < 0 || nx >= RogueSharedData::MAP_W || ny < 0 || ny >= RogueSharedData::MAP_H) continue;

                if (nx == _data->player.x && ny == _data->player.y) {
                    int damage = m.attack - _data->player.defense;
                    if (damage < 1) damage = 1; 

                    _data->player.hp -= damage;
                    playerHit = true;
                    break; 
                }
                else if (_data->map[ny][nx] != TileType::WALL && !_getMonsterAt(nx, ny)) {
                    m.x = nx;
                    m.y = ny;
                }
            }
        }

        // Boss Summoning Ability
        if (m.type == MonsterType::BOSS && m.alert && random(100) < 15) {
            // Find an inactive monster slot
            for (auto& newM : _data->monsters) {
                if (!newM.active) {
                    newM.active = true;
                    newM.type = (random(2) == 0) ? MonsterType::SKELETON : MonsterType::GOBLIN;
                    newM.x = m.x + (random(3) - 1);
                    newM.y = m.y + (random(3) - 1);
                    
                    // Validate position
                    if (newM.x >= 0 && newM.x < RogueSharedData::MAP_W && 
                        newM.y >= 0 && newM.y < RogueSharedData::MAP_H &&
                        _data->map[newM.y][newM.x] == TileType::FLOOR && 
                        (newM.x != _data->player.x || newM.y != _data->player.y) &&
                        !_getMonsterAt(newM.x, newM.y)) {
                        newM.hp = 8 + _data->currentDepth;
                        newM.maxHp = newM.hp;
                        newM.attack = 2 + (_data->currentDepth / 3);
                        newM.alert = true;
                        
                        _camera->shake(5);
                        ctx.beep(200, 100);
                        snprintf(_hudMessage, sizeof(_hudMessage), "Boss Summons!");
                        _hudMessageTimer = 60;
                    } else {
                        newM.active = false; // Cancel if spot is invalid
                    }
                    break;
                }
            }
        }
    }

    if (playerHit) {
        ctx.sfxDeath(); 
        _camera->shake(6);
        
        if (_data->player.hp <= 0) {
            if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
            ctx.sfxDeath();
            sm.emit(ctx, Event::GAME_OVER);
        }
    } else {
        ctx.beep(400, 10); 
    }
}

bool RoguePlayScene::_processTurn(Console& ctx, SceneManager& sm, int dx, int dy) {
    auto finalizeTurn = [&]() {
        _data->turnCount++; 
        // Passive HP Regeneration (1 HP every 20 turns)
        if (_data->turnCount % 20 == 0 && _data->player.hp < _data->player.maxHp) {
            _data->player.hp++;
        }
        return true;
    };

    if (dx == 0 && dy == 0) {
        snprintf(_hudMessage, sizeof(_hudMessage), "Waiting...");
        _hudMessageTimer = 20;
        return finalizeTurn();
    }

    int targetX = _data->player.x + dx;
    int targetY = _data->player.y + dy;

    if (targetX < 0 || targetX >= RogueSharedData::MAP_W || targetY < 0 || targetY >= RogueSharedData::MAP_H) return false;

    TileType targetTile = _data->map[targetY][targetX];
    if (targetTile == TileType::WALL) return false; 

    Monster* targetMonster = _getMonsterAt(targetX, targetY);
    
    if (targetTile == TileType::KEY) {
        _data->keys++;
        _data->map[targetY][targetX] = TileType::FLOOR;
        snprintf(_hudMessage, sizeof(_hudMessage), "Found a Key!");
        _hudMessageTimer = 60;
        ctx.beep(1200, 50);
    }

    if (targetMonster) {
        int dmg = _data->player.attack;
        bool crit = false;
        
        if (!targetMonster->alert) {
            crit = true; // Guaranteed Sneak Attack!
            targetMonster->alert = true;
            snprintf(_hudMessage, sizeof(_hudMessage), "Sneak Attack!");
            _hudMessageTimer = 40;
        } else {
            crit = (random(100) < 20);
        }
        
        if (crit) dmg *= 2;
        
        targetMonster->hp -= dmg;
        _camera->shake(crit ? 6 : 3); 
        _spawnHitEffect(targetX, targetY);
        ctx.beep(crit ? 1500 : 1000, 20); 
        
        if (targetMonster->hp <= 0) {
            targetMonster->active = false;
            _data->player.xp += targetMonster->maxHp;
            
            bool leveledUp = false;
            while (_data->player.xp >= _data->player.level * 10) {
                _data->player.xp -= _data->player.level * 10;
                _data->player.level++;
                _data->player.maxHp += 5;
                _data->player.hp = _data->player.maxHp; 
                _data->player.baseAttack += 1;
                recalcStats(_data);
                leveledUp = true;
            }
            if (leveledUp) {
                snprintf(_hudMessage, sizeof(_hudMessage), "LEVEL UP!");
                _hudMessageTimer = 60;
                ctx.beep(800, 100); ctx.beep(1200, 150);
            }
            if (targetMonster->type == MonsterType::BOSS) {
                _data->map[targetMonster->y][targetMonster->x] = TileType::STAIRS_DOWN;
                if (_data->map[targetMonster->y + 1][targetMonster->x] == TileType::FLOOR) {
                    _data->map[targetMonster->y + 1][targetMonster->x] = TileType::CHEST;
                }
                snprintf(_hudMessage, sizeof(_hudMessage), "Boss Defeated!");
                _hudMessageTimer = 80;
                ctx.beep(1500, 200);
            }
        }
        return finalizeTurn();
    }
    else if (targetTile == TileType::LOCKED_DOOR) {
        if (_data->keys > 0) {
            _data->keys--;
            _data->map[targetY][targetX] = TileType::CORRIDOR; // Remove door
            snprintf(_hudMessage, sizeof(_hudMessage), "Door Unlocked!");
            _hudMessageTimer = 60;
            ctx.beep(1000, 100);
            return finalizeTurn(); // Takes a turn to unlock
        } else {
            snprintf(_hudMessage, sizeof(_hudMessage), "Locked!");
            _hudMessageTimer = 40;
            ctx.beep(150, 100);
            return false; // Free action if you bump it without a key
        }
    }
    else if (targetTile == TileType::CHEST) {
        if (random(100) < 15) {
            _data->map[targetY][targetX] = TileType::FLOOR; 
            snprintf(_hudMessage, sizeof(_hudMessage), "It's a MIMIC!");
            _hudMessageTimer = 60;
            ctx.beep(200, 150);
            _camera->shake(8);
            
            for (auto& m : _data->monsters) {
                if (!m.active) {
                    m.x = targetX; m.y = targetY;
                    m.active = true;
                    m.hp = 10 + (_data->currentDepth * 3);
                    m.maxHp = m.hp;
                    m.attack = _data->player.attack + 1; 
                    m.type = MonsterType::GOBLIN; 
                    break;
                }
            }
            return finalizeTurn(); 
        }

        int roll = random(100);
        ItemType itemToGive = ItemType::NONE;
        
        if (roll < 20) { itemToGive = ItemType::POTION; }
        else if (roll < 30) { itemToGive = ItemType::ELIXIR; }
        else if (roll < 45) { itemToGive = ItemType::SCROLL_UPGRADE; }
        else if (roll < 70) { 
            if (_data->currentDepth < 3) itemToGive = (random(2)==0) ? ItemType::DAGGER : ItemType::SWORD;
            else if (_data->currentDepth < 6) itemToGive = (random(2)==0) ? ItemType::SWORD : ItemType::AXE;
            else itemToGive = ItemType::AXE;
        } else if (roll < 90) {
            if (_data->currentDepth < 3) itemToGive = (random(2)==0) ? ItemType::LEATHER : ItemType::CHAINMAIL;
            else if (_data->currentDepth < 6) itemToGive = (random(2)==0) ? ItemType::CHAINMAIL : ItemType::PLATE;
            else itemToGive = ItemType::PLATE;
        }
        
        if (itemToGive != ItemType::NONE) {
            const char* itemName = getItemName(itemToGive);
            bool added = false;
            
            if (itemToGive == ItemType::POTION || itemToGive == ItemType::ELIXIR || itemToGive == ItemType::SCROLL_UPGRADE) {
                for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                    if(_data->inventory[i].type == itemToGive) {
                        _data->inventory[i].count++;
                        added = true; 
                        break;
                    }
                }
            }
            
            if (!added) {
                for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                    if(_data->inventory[i].type == ItemType::NONE) {
                        _data->inventory[i].type = itemToGive;
                        _data->inventory[i].count = 1;
                        _data->inventory[i].level = 0;
                        added = true; 
                        break;
                    }
                }
            }
            if(!added) {
                snprintf(_hudMessage, sizeof(_hudMessage), "Pack Full!");
                _hudMessageTimer = 60;
                ctx.beep(150, 100);
                return false; // Do not consume chest
            }
            _data->map[targetY][targetX] = TileType::FLOOR; 
            snprintf(_hudMessage, sizeof(_hudMessage), "Got %s!", itemName);
            _hudMessageTimer = 60;
            ctx.beep(800, 40); ctx.beep(1200, 60);
        } else {
            _data->map[targetY][targetX] = TileType::FLOOR; 
            int amount = 15 * _data->currentDepth;
            _data->gold += amount;
            snprintf(_hudMessage, sizeof(_hudMessage), "Found %d Gold", amount);
            _hudMessageTimer = 60;
            ctx.beep(1200, 20); ctx.beep(1500, 40);
        }
        return finalizeTurn();
    }
    else if (targetTile == TileType::MERCHANT) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_2); // Shop
        return false; 
    }
    else {
        _data->player.x = targetX;
        _data->player.y = targetY;

        if (targetTile == TileType::TALL_GRASS) {
            _data->map[targetY][targetX] = TileType::FLOOR; // Trample the grass
            if (random(100) < 15 && _data->player.hp < _data->player.maxHp) {
                _data->player.hp++;
                snprintf(_hudMessage, sizeof(_hudMessage), "Dewdrop: +1 HP");
                _hudMessageTimer = 40;
                ctx.beep(1000, 30);
            }
        }

        if (targetTile == TileType::SPIKE) {
            _data->player.hp -= 2;
            _camera->shake(4);
            ctx.sfxDeath();
            snprintf(_hudMessage, sizeof(_hudMessage), "Stepped on Spikes!");
            _hudMessageTimer = 60;
            
            if (_data->player.hp <= 0) {
                if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
                sm.emit(ctx, Event::GAME_OVER);
                return true;
            }
        }

        if (targetTile == TileType::STAIRS_DOWN) {
            // Play a descending tone
            ctx.beep(400, 100); ctx.beep(300, 150); 
            _descending = true;
            _fadeTimer = 20; // 20 frames for the cinematic bars to close in
            return finalizeTurn();
        }
        return finalizeTurn();
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RogueShopScene 
// ═════════════════════════════════════════════════════════════════════════════

void RogueShopScene::onEnter(Console& ctx) { 
    _cursor = 0; 
    _msgTimer = 0;
    _introFrames = 0;
}

void RogueShopScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (_introFrames < 10) _introFrames++;
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
        return;
    }

    if (ctx.justPressed(Btn::UP) || ctx.repeat(Btn::UP)) {
        if (_cursor > 0) { _cursor--; ctx.sfxMenuNav(); }
    }
    if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) {
        if (_cursor < 4) { _cursor++; ctx.sfxMenuNav(); }
    }

    if (_msgTimer > 0) _msgTimer--;

    if (ctx.justPressed(Btn::A)) {
        int costHealth = 20;
        int costPotion = 40;
        int costDarts  = 30;
        int costScroll = 150;
        
        auto giveItem = [&](ItemType type, int count) -> bool {
            for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                if(_data->inventory[i].type == type) {
                    _data->inventory[i].count += count;
                    return true;
                }
            }
            for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                if(_data->inventory[i].type == ItemType::NONE) {
                    _data->inventory[i].type = type;
                    _data->inventory[i].count = count;
                    _data->inventory[i].level = 0;
                    return true;
                }
            }
            return false;
        };

        if (_cursor == 0) { // Buy Health
            if (_data->player.hp >= _data->player.maxHp) {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "ALREADY FULL!");
                _msgTimer = 40;
            } else if (_data->gold >= costHealth) {
                _data->gold -= costHealth;
                _data->player.hp = gclamp(_data->player.hp + 5, 0, _data->player.maxHp);
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "HEALED HP!");
                _msgTimer = 40;
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 1) { // Buy Potion
            if (_data->gold >= costPotion) {
                if (giveItem(ItemType::POTION, 1)) {
                    _data->gold -= costPotion;
                    ctx.sfxPoint();
                    snprintf(_msg, sizeof(_msg), "BOUGHT POTION!");
                    _msgTimer = 40;
                } else {
                    ctx.beep(150, 100);
                    snprintf(_msg, sizeof(_msg), "PACK FULL!");
                    _msgTimer = 40;
                }
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 2) { // Buy Darts
            if (_data->gold >= costDarts) {
                if (giveItem(ItemType::THROWING_DART, 3)) {
                    _data->gold -= costDarts;
                    ctx.sfxPoint();
                    snprintf(_msg, sizeof(_msg), "BOUGHT DARTS!");
                    _msgTimer = 40;
                } else {
                    ctx.beep(150, 100);
                    snprintf(_msg, sizeof(_msg), "PACK FULL!");
                    _msgTimer = 40;
                }
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 3) { // Buy Upg Scroll
            if (_data->gold >= costScroll) {
                if (giveItem(ItemType::SCROLL_UPGRADE, 1)) {
                    _data->gold -= costScroll;
                    ctx.sfxPoint();
                    snprintf(_msg, sizeof(_msg), "BOUGHT SCROLL!");
                    _msgTimer = 40;
                } else {
                    ctx.beep(150, 100);
                    snprintf(_msg, sizeof(_msg), "PACK FULL!");
                    _msgTimer = 40;
                }
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else { // Exit
            ctx.sfxMenuNav();
            sm.emit(ctx, Event::RESUME);
        }
    }
}

void RogueShopScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int yOff = lerpi(Console::H, 0, _introFrames, 10);
    int bx = 4, by = 4 + yOff, bw = 120, bh = 56; 

    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 12); 
    
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.setDrawColor(0);
    ctx.drawStr(bx + 4, by + 9, "MERCHANT");

    char gBuf[16]; 
    snprintf(gBuf, sizeof(gBuf), "%u gold", _data->gold);
    int gw = ctx.strWidth(gBuf);
    ctx.drawStr(bx + bw - gw - 4, by + 9, gBuf);

    ctx.setDrawColor(1);
    
    ctx.drawStr(bx + 14, by + 21, "Heal HP   (20g)");
    ctx.drawStr(bx + 14, by + 29, "Potion    (40g)");
    ctx.drawStr(bx + 14, by + 37, "Darts x3  (30g)");
    ctx.drawStr(bx + 14, by + 45, "Upg Scrl (150g)");
    ctx.drawStr(bx + 14, by + 53, "Exit Shop");

    ctx.drawStr(bx + 4, by + 21 + (_cursor * 8), ">");

    if (_msgTimer > 0) {
        ctx.setDrawColor(0);
        ctx.drawBox(bx + 2, by + bh - 14, bw - 4, 12);
        ctx.setDrawColor(1);
        
        int msgW = ctx.strWidth(_msg);
        ctx.drawStr(bx + (bw - msgW) / 2, by + bh - 5, _msg);
    }
}

void RoguePlayScene::draw(Console& ctx) {
    drawDungeon(ctx, 0, 0); 
    
    ctx.setFont(u8g2_font_5x7_tf);
    
    if (_hudMessageTimer > 0) {
        int floatY = lerpi(0, 12, _hudMessageTimer, 60);
        bool visible = (_hudMessageTimer > 15) || (_hudMessageTimer % 2 == 0);
        
        if (visible) {
            int w = ctx.strWidth(_hudMessage);
            ctx.setDrawColor(0);
            ctx.drawBox((Console::W - w) / 2 - 2, floatY, w + 4, 9);
            ctx.setDrawColor(1);
            ctx.drawStr((Console::W - w) / 2, floatY + 7, _hudMessage);
        }
    } else {
        char topBuf[32];
        snprintf(topBuf, sizeof(topBuf), "%d/%d L:%d", _data->player.hp, _data->player.maxHp, _data->player.level);
        int topW = ctx.strWidth(topBuf);
        
        ctx.setDrawColor(0);
        ctx.drawBox(0, 0, 10 + topW + 2, 10);
        ctx.setDrawColor(1);
        ctx.drawBitmap(1, 1, 1, 8, spr_icon_heart);
        ctx.drawStr(11, 7, topBuf);

        char atkBuf[8], defBuf[8];
        snprintf(atkBuf, sizeof(atkBuf), "%d", _data->player.attack);
        snprintf(defBuf, sizeof(defBuf), "%d", _data->player.defense);
        
        int atkW = ctx.strWidth(atkBuf);
        int defW = ctx.strWidth(defBuf);
        int trTotalW = 8 + 2 + atkW + 4 + 8 + 2 + defW;
        int trStartX = Console::W - trTotalW - 2;
        
        ctx.setDrawColor(0);
        ctx.drawBox(trStartX, 0, trTotalW + 2, 10);
        ctx.setDrawColor(1);
        
        ctx.drawBitmap(trStartX + 1, 1, 1, 8, spr_icon_sword);
        ctx.drawStr(trStartX + 11, 7, atkBuf);
        
        int shieldX = trStartX + 11 + atkW + 4;
        ctx.drawBitmap(shieldX, 1, 1, 8, spr_icon_shield);
        ctx.drawStr(shieldX + 10, 7, defBuf);
    }

    char depBuf[8], goldBuf[16];
    snprintf(depBuf, sizeof(depBuf), "%d", _data->currentDepth);
    snprintf(goldBuf, sizeof(goldBuf), "%d", _data->gold);
    
    int depW = ctx.strWidth(depBuf);
    int goldW = ctx.strWidth(goldBuf);
    int brTotalW = 8 + 2 + depW + 4 + 8 + 2 + goldW;
    int brStartX = Console::W - brTotalW - 2;
    int botY = Console::H - 10;
    
    ctx.setDrawColor(0);
    ctx.drawBox(brStartX, botY, brTotalW + 2, 10);
    ctx.setDrawColor(1);
    
    ctx.drawBitmap(brStartX + 1, botY + 1, 1, 8, spr_icon_depth);
    ctx.drawStr(brStartX + 11, botY + 7, depBuf);
    
    int coinX = brStartX + 11 + depW + 4;
    ctx.drawBitmap(coinX, botY + 1, 1, 8, spr_icon_coin);
    ctx.drawStr(coinX + 10, botY + 7, goldBuf);

    // Draw Keys
    if (_data->keys > 0) {
        char keyBuf[8]; snprintf(keyBuf, sizeof(keyBuf), "x%d", _data->keys);
        int kw = ctx.strWidth(keyBuf);
        int keyX = brStartX - 10 - kw - 4;
        ctx.setDrawColor(0);
        ctx.drawBox(keyX, botY, 10 + kw + 2, 10);
        ctx.setDrawColor(1);
        ctx.drawBitmap(keyX + 1, botY + 1, 1, 8, spr_rogue_key);
        ctx.drawStr(keyX + 11, botY + 7, keyBuf);
    }

    // Draw Cinematic Fade Effect
    if (_descending || _fadeTimer < 0) {
        // Calculate progress from 0 (open) to 20 (closed)
        int progress = _descending ? (20 - _fadeTimer) : abs(_fadeTimer);
        
        // Calculate how tall the bars should be (max is half the screen height, plus 1 to overlap perfectly)
        int barHeight = ((Console::H / 2) + 1) * progress / 20;
        
        ctx.setCamera(nullptr); // Unset camera to stick bars to the screen
        ctx.setDrawColor(0);    // Black
        ctx.drawBox(0, 0, Console::W, barHeight);                             // Top bar wiping down
        ctx.drawBox(0, Console::H - barHeight, Console::W, barHeight);        // Bottom bar wiping up
        ctx.setDrawColor(1);    // Reset to White
    }
}

void RoguePlayScene::drawDungeon(Console& ctx, int ox, int oy) const {
    ctx.setCamera(_camera);

    int viewportTilesX = (Console::W / 8) + 1;
    int viewportTilesY = (Console::H / 8) + 1;
    
    int startCol = _camera->x / 8;
    int startRow = _camera->y / 8;

    for (int y = 0; y <= viewportTilesY; y++) {
        for (int x = 0; x <= viewportTilesX; x++) {
            int mapX = startCol + x;
            int mapY = startRow + y;

            if (mapX < 0 || mapX >= RogueSharedData::MAP_W || mapY < 0 || mapY >= RogueSharedData::MAP_H) continue;

            int distX = abs(mapX - _data->player.x);
            int distY = abs(mapY - _data->player.y);
            bool inSight = (distX * distX + distY * distY <= 20); 
            
            if (inSight) _data->explored[mapY][mapX] = true;
            if (!_data->explored[mapY][mapX]) continue; 

            int renderX = (mapX * 8);
            int renderY = (mapY * 8); 

            TileType t = _data->map[mapY][mapX];

            if (t == TileType::WALL) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_wall);
            } else if (t == TileType::FLOOR || t == TileType::CORRIDOR) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_floor);
            } else if (t == TileType::TALL_GRASS) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_grass);
            } else if (t == TileType::STAIRS_DOWN) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_stairs);
            } else if (t == TileType::CHEST) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_chest);
            } else if (t == TileType::MERCHANT) {            
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_merchant);
            } else if (t == TileType::SPIKE) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_spike);
            } else if (t == TileType::KEY) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_key);
            } else if (t == TileType::LOCKED_DOOR) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_door);
            }

            Monster* m = _getMonsterAt(mapX, mapY);
            if (m && inSight) {
                ctx.setDrawColor(0);
                ctx.drawBox(renderX, renderY, 8, 8);
                ctx.setDrawColor(1);
                
                if (m->type == MonsterType::RAT) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_rat);
                else if (m->type == MonsterType::GOBLIN) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_goblin);
                else if (m->type == MonsterType::BAT) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_bat);
                else if (m->type == MonsterType::SKELETON) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_skeleton);
                else if (m->type == MonsterType::ORC) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_orc);
                else if (m->type == MonsterType::TROLL) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_troll);
                else if (m->type == MonsterType::BOSS) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_boss);
                
                // Draw sleeping indicator 'z' if unaware of player
                if (!m->alert && (millis() / 500) % 2 == 0) {
                    ctx.setFont(u8g2_font_5x7_tf);
                    ctx.drawStr(renderX + 2, renderY - 1, "z");
                }

                // Draw HP Bar if damaged
                if (m->hp < m->maxHp) {
                    int hpWidth = max(1, (m->hp * 6) / m->maxHp);
                    ctx.setDrawColor(0);
                    ctx.drawHLine(renderX + 1, renderY + 7, 6);
                    ctx.setDrawColor(1);
                    ctx.drawHLine(renderX + 1, renderY + 7, hpWidth);
                }
            } 
            else if (mapX == _data->player.x && mapY == _data->player.y) {
                ctx.setDrawColor(0);
                ctx.drawBox(renderX, renderY, 8, 8);
                ctx.setDrawColor(1);
                
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_player);
            }

            if (!inSight) {
                ctx.setDrawColor(0); 
                for(int dy=0; dy<8; dy++) {
                    for(int dx=0; dx<8; dx++) {
                        if ((dx+dy)%2 == 0) ctx.drawPixel(renderX+dx, renderY+dy);
                    }
                }
                ctx.setDrawColor(1);
            }
        }
    }

    _particles->draw(ctx);

    if (isAiming) {
        ctx.setDrawColor(1);
        int px = (_data->player.x * 8) + 4;
        int py = (_data->player.y * 8) + 4;
        int cx = (aimX * 8) + 4;
        int cy = (aimY * 8) + 4;
        
        // Draw dotted trajectory line
        int steps = max(abs(cx - px), abs(cy - py)) / 2;
        if (steps > 0) {
            for (int i = 0; i <= steps; i+=2) {
                int lx = px + (cx - px) * i / steps;
                int ly = py + (cy - py) * i / steps;
                ctx.drawPixel(lx, ly);
            }
        }
        
        ctx.drawFrame(aimX * 8, aimY * 8, 8, 8);
    }

    ctx.setCamera(nullptr);
}

// ═════════════════════════════════════════════════════════════════════════════
// RoguePauseScene & RogueDeadScene 
// ═════════════════════════════════════════════════════════════════════════════

void RoguePauseScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (_introFrames < 10) _introFrames++;
    
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
    }
}

void RoguePauseScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int yOff = lerpi(-30, 0, _introFrames, 8);
    int bx = 30, by = 22 + yOff, bw = 68, bh = 24;

    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 14);

    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.setDrawColor(0);
    int tw = ctx.strWidth("PAUSED");
    ctx.drawStr(bx + (bw - tw) / 2, by + 11, "PAUSED");

    ctx.setDrawColor(1);
    ctx.setFont(u8g2_font_5x7_tf);
    tw = ctx.strWidth("[B] Resume");
    ctx.drawStr(bx + (bw - tw) / 2, by + 21, "[B] Resume");
}

void RogueDeadScene::onEnter(Console& ctx) { 
    _frame = 0; 
    ctx.removeSave("gamestate");
}

void RogueDeadScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // PlayScene
    }
}

void RogueDeadScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int bx = 20, by = 20, bw = 88, bh = 28;

    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 14);

    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.setDrawColor(0);
    int tw = ctx.strWidth("YOU DIED");
    ctx.drawStr(bx + (bw - tw) / 2, by + 11, "YOU DIED");
    
    ctx.setDrawColor(1);
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        tw = ctx.strWidth("A to restart");
        ctx.drawStr(bx + (bw - tw) / 2, by + 24, "A to restart");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RogueInventoryScene 
// ═════════════════════════════════════════════════════════════════════════════

void RogueInventoryScene::_cleanInventory() {
    int insert = 0;
    for (int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
        if (_data->inventory[i].type != ItemType::NONE) {
            if (insert != i) {
                _data->inventory[insert] = _data->inventory[i];
                _data->inventory[i].type = ItemType::NONE;
                _data->inventory[i].level = 0;
                _data->inventory[i].count = 0;
            }
            insert++;
        }
    }
}

void RogueInventoryScene::onEnter(Console& ctx) {
    _cursor = 0;
    _msgTimer = 0;
    _upgrading = false;
}

void RogueInventoryScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }

    if (_upgrading) {
        if (ctx.justPressed(Btn::UP) || ctx.justPressed(Btn::DOWN)) {
            _upgradeSelect = (_upgradeSelect == 0) ? 1 : 0;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU2)) {
            _upgrading = false;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::A)) {
            Item* target = (_upgradeSelect == 0) ? &_data->equippedWeapon : &_data->equippedArmor;
            if (target->type != ItemType::NONE) {
                target->level++;
                recalcStats(_data);
                
                _data->inventory[_cursor].count--;
                if (_data->inventory[_cursor].count == 0) _data->inventory[_cursor].type = ItemType::NONE;
                _cleanInventory();
                
                snprintf(_msg, sizeof(_msg), "Upgraded!");
                _msgTimer = 60;
                ctx.sfxPoint();
                _upgrading = false;
            } else {
                snprintf(_msg, sizeof(_msg), "Slot Empty!");
                _msgTimer = 40;
                ctx.beep(150, 100);
            }
        }
        return;
    }

    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.emit(ctx, Event::RESUME);
        return;
    }

    if (ctx.justPressed(Btn::UP) || ctx.repeat(Btn::UP)) {
        if (_cursor > 0) { _cursor--; ctx.sfxMenuNav(); }
    }
    if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) {
        if (_cursor < RogueSharedData::MAX_INVENTORY - 1) { _cursor++; ctx.sfxMenuNav(); }
    }
    
    if (_msgTimer > 0) _msgTimer--;

    // --- Discard Item ---
    if (ctx.justPressed(Btn::RIGHT)) {
        if (_data->inventory[_cursor].type != ItemType::NONE) {
            _data->inventory[_cursor].count--;
            if (_data->inventory[_cursor].count <= 0) _data->inventory[_cursor].type = ItemType::NONE;
            _cleanInventory();
            snprintf(_msg, sizeof(_msg), "Discarded!");
            _msgTimer = 40;
            ctx.beep(200, 100); // Crunch sound
        }
    }

    if (ctx.justPressed(Btn::A)) {
        Item& item = _data->inventory[_cursor];
        if (item.type != ItemType::NONE) {
            bool consumed = false;
            
            if (item.type == ItemType::POTION) {
                _data->player.hp = gclamp(_data->player.hp + 15, 0, _data->player.maxHp);
                snprintf(_msg, sizeof(_msg), "Healed 15 HP!");
                _data->inventoryTurnUsed = true;
                consumed = true;
            } else if (item.type == ItemType::ELIXIR) {
                _data->player.maxHp += 5;
                _data->player.hp += 5;
                snprintf(_msg, sizeof(_msg), "Max HP +5!");
                _data->inventoryTurnUsed = true;
                consumed = true;
            } else if (item.type == ItemType::SCROLL_UPGRADE) {
                if (_data->equippedWeapon.type == ItemType::NONE && _data->equippedArmor.type == ItemType::NONE) {
                    snprintf(_msg, sizeof(_msg), "No Gear!");
                    _msgTimer = 60;
                    ctx.beep(150, 100);
                } else {
                    _upgrading = true;
                    _upgradeSelect = 0;
                    ctx.sfxMenuEnter();
                }
            } else if (item.type == ItemType::DAGGER || item.type == ItemType::SWORD || item.type == ItemType::AXE) {
                if (_data->equippedWeapon.type == item.type) {
                    _data->equippedWeapon.level += item.level + 1; // Merge!
                    recalcStats(_data);
                    snprintf(_msg, sizeof(_msg), "Weapons Merged!");
                    ctx.beep(1200, 40); ctx.beep(1500, 60);
                    consumed = true;
                    _data->inventoryTurnUsed = true;
                } else {
                    Item temp = _data->equippedWeapon;
                    _data->equippedWeapon = item;
                    _data->inventory[_cursor] = temp;
                    recalcStats(_data);
                    snprintf(_msg, sizeof(_msg), "Equipped Weapon!");
                    _data->inventoryTurnUsed = true;
                    ctx.sfxPoint();
                    _msgTimer = 60;
                }
            } else if (item.type == ItemType::LEATHER || item.type == ItemType::CHAINMAIL || item.type == ItemType::PLATE) {
                if (_data->equippedArmor.type == item.type) {
                    _data->equippedArmor.level += item.level + 1; // Merge!
                    recalcStats(_data);
                    snprintf(_msg, sizeof(_msg), "Armors Merged!");
                    ctx.beep(1200, 40); ctx.beep(1500, 60);
                    consumed = true;
                    _data->inventoryTurnUsed = true;
                } else {
                    Item temp = _data->equippedArmor;
                    _data->equippedArmor = item;
                    _data->inventory[_cursor] = temp;
                    recalcStats(_data);
                    snprintf(_msg, sizeof(_msg), "Equipped Armor!");
                    _data->inventoryTurnUsed = true;
                    ctx.sfxPoint();
                    _msgTimer = 60;
                }
            } else if (item.type == ItemType::THROWING_DART) {
                sm.pop(ctx);
                RoguePlayScene* play = (RoguePlayScene*)sm.current(); // Assuming Play is under inventory
                play->isAiming = true;
                play->aimX = _data->player.x;
                play->aimY = _data->player.y;
                return;
            }
            
            if (consumed) {
                item.count--;
                if (item.count == 0) item.type = ItemType::NONE;
                _msgTimer = 60;
                ctx.sfxPoint();
            }
            _cleanInventory();
        }
    }
}

void RogueInventoryScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int bx = 2, by = 2, bw = 124, bh = 60;
    
    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 9);
    
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.setDrawColor(0);
    ctx.drawStr(bx + 4, by + 7, "PACK  [>]Drop");
    int exitW = ctx.strWidth("[B]Exit");
    ctx.drawStr(bx + bw - exitW - 2, by + 7, "[B]Exit");
    
    ctx.setDrawColor(1);

    // Draw Equipped Gear with Stats
    char wStr[32];
    if (_data->equippedWeapon.type != ItemType::NONE) {
        snprintf(wStr, sizeof(wStr), "W: %s+%d (+%d)", getItemName(_data->equippedWeapon.type), _data->equippedWeapon.level, getWeaponAttack(_data->equippedWeapon.type) + _data->equippedWeapon.level);
    } else {
        strcpy(wStr, "W: - None -");
    }
    ctx.drawStr(bx + 4, by + 16, wStr);

    char aStr[32];
    if (_data->equippedArmor.type != ItemType::NONE) {
        snprintf(aStr, sizeof(aStr), "A: %s+%d (+%d)", getItemName(_data->equippedArmor.type), _data->equippedArmor.level, getArmorDefense(_data->equippedArmor.type) + _data->equippedArmor.level);
    } else {
        strcpy(aStr, "A: - None -");
    }
    ctx.drawStr(bx + 4, by + 24, aStr);
    
    ctx.drawHLine(bx, by + 27, bw);

    // Draw 6 inventory items in a 2-Column x 3-Row grid
    for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
        int col = i % 2;
        int row = i / 2;
        int itemX = bx + 4 + (col * 60); // Split 124 width into two 60px columns
        int itemY = by + 36 + (row * 9);
        
        if (i == _cursor) {
            ctx.drawStr(itemX, itemY, ">");
        }
        
        Item& item = _data->inventory[i];
        char nameBuf[32];
        if (item.type == ItemType::NONE) {
            strcpy(nameBuf, "Empty");
        } else {
            // Abbreviate slightly so it fits in the column
            const char* shortName = getItemName(item.type);
            if (item.type == ItemType::CHAINMAIL) shortName = "Chain";
            else if (item.type == ItemType::LEATHER) shortName = "Lthr";
            else if (item.type == ItemType::DAGGER) shortName = "Dagr";
            else if (item.type == ItemType::SWORD) shortName = "Swrd";
            else if (item.type == ItemType::SCROLL_UPGRADE) shortName = "Upg";

            int atk = getWeaponAttack(item.type);
            int def = getArmorDefense(item.type);
            
            if (atk > 0 || def > 0) snprintf(nameBuf, sizeof(nameBuf), "%s+%d", shortName, item.level);
            else snprintf(nameBuf, sizeof(nameBuf), "%s x%d", shortName, item.count);
        }
        
        ctx.drawStr(itemX + 6, itemY, nameBuf);
    }

    if (_upgrading) {
        ctx.setDrawColor(0);
        ctx.drawBox(24, 14, 80, 36);
        ctx.setDrawColor(1);
        ctx.drawFrame(24, 14, 80, 36);
        ctx.drawStr(28, 23, "Upgrade:");
        ctx.drawStr(38, 34, "Weapon");
        ctx.drawStr(38, 44, "Armor");
        ctx.drawStr(28, 34 + (_upgradeSelect * 10), ">");
    }

    if (_msgTimer > 0) {
        ctx.setDrawColor(0);
        ctx.drawBox(bx + 2, by + bh - 12, bw - 4, 10);
        ctx.setDrawColor(1);
        int mw = ctx.strWidth(_msg);
        ctx.drawStr(bx + (bw - mw)/2, by + bh - 4, _msg);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// TinyRogueGame - OS Registration Hook
// ═════════════════════════════════════════════════════════════════════════════

void TinyRogueGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();
    
    bool loaded = false;
    if (ctx.hasSave("gamestate")) {
        size_t bytesRead = ctx.loadBytes("gamestate", &_data, sizeof(RogueSharedData));
        if (bytesRead == sizeof(RogueSharedData) && _data.player.hp > 0) {
            loaded = true;
        }
    }

    _play.setData(&_data);
    if (loaded) _play.resumeSavedGame();
    _play.setEngine(&_camera, &_particles);
    
    _shop.setData(&_data);
    _dead.setData(&_data);

    // Event Registry Mapping
    _sm.onEvent(Event::QUIT,      SceneManager::CLEAR);
    _sm.onEvent(Event::PAUSE,     SceneManager::PUSH, &_pause);
    _sm.onEvent(Event::RESUME,    SceneManager::POP);
    _sm.onEvent(Event::GAME_OVER, SceneManager::REPLACE, &_dead);
    _inventory.setData(&_data);
    _sm.onEvent(Event::CUSTOM_1,  SceneManager::REPLACE, &_play); // Start/Restart Game
    _sm.onEvent(Event::CUSTOM_2,  SceneManager::PUSH, &_shop);    // Enter Shop
    _sm.onEvent(Event::CUSTOM_3,  SceneManager::PUSH, &_inventory); // Enter Inventory

    _sm.replace(&_title, ctx);
}

void TinyRogueGame::onExit(Console& ctx) {
    ctx.saveHiScore(_data.hiScore);
    if (_data.player.hp > 0) {
        ctx.saveBytes("gamestate", &_data, sizeof(RogueSharedData));
    } else {
        ctx.removeSave("gamestate");
    }
}

void TinyRogueGame::update(Console& ctx, float dt) { 
    _camera.update();
    _particles.update();
    _sm.update(ctx, dt); 
}
void TinyRogueGame::draw(Console& ctx)   { _sm.draw(ctx); }

bool        TinyRogueGame::isRunning() const { return !_sm.empty(); }
const char* TinyRogueGame::getName()   const { return "Tiny Rogue"; }
const uint8_t* TinyRogueGame::getCoverArt() const { return spr_tinyrogue_cover; }

REGISTER_GAME(TinyRogueGame);
