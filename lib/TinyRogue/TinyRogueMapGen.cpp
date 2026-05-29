#include "TinyRogueMapGen.h"
#include "GameUtils.h"
#include <cstring>

namespace TinyRogueMapGen {

void generateMap(RogueSharedData* _data) {
    // 1. Reset Fog of War
    memset(_data->explored, 0, sizeof(_data->explored));

    if (_data->currentDepth % 5 == 0) _data->currentBiome = Biome::BOSS_ARENA;
    else if (_data->currentDepth < 5) _data->currentBiome = Biome::SEWERS;
    else if (_data->currentDepth < 10) _data->currentBiome = Biome::PRISON;
    else _data->currentBiome = Biome::DEEP_CAVES;

    // Roll for Level Mutator (25% chance, except on Boss levels)
    _data->currentMutator = LevelMutator::NONE;
    if (_data->currentDepth % 5 != 0 && random(100) < 25) {
        int m = random(6);
        if (m == 0) _data->currentMutator = LevelMutator::PITCH_BLACK;
        else if (m == 1) _data->currentMutator = LevelMutator::INFESTED;
        else if (m == 2) _data->currentMutator = LevelMutator::TREASURE_TROVE;
        else if (m == 3) _data->currentMutator = LevelMutator::FLOODED;
        else if (m == 4) _data->currentMutator = LevelMutator::OVERGROWN;
        else if (m == 5) _data->currentMutator = LevelMutator::LABYRINTH;
    }

    // 2. Biomes and Boss Arenas
    bool isBSP = false; // FIX: Track if map is BSP
    if (_data->currentDepth % 5 == 0) {
        _data->currentBiome = Biome::BOSS_ARENA;
        generateBossMap(_data);
    } else {
        if (_data->currentDepth < 5) _data->currentBiome = Biome::SEWERS;
        else if (_data->currentDepth < 10) _data->currentBiome = Biome::PRISON;
        else _data->currentBiome = Biome::DEEP_CAVES;

        if (_data->currentMutator == LevelMutator::LABYRINTH) {
            generateMazeMap(_data);
        } else if (_data->currentDepth < 5) {
            generateCaveMap(_data);
        } else if (_data->currentDepth < 10) {
            generateBSPMap(_data);
            isBSP = true;
        } else {
            if (random(100) < 50) { generateBSPMap(_data); isBSP = true; }
            else generateCaveMap(_data);
        }
    }

    // 3. Shared Spawning & Setup
    _data->keys = 0;
    
    // FIX: Only spawn doors on non-boss levels that use the BSP generator.
    // Cave and Maze maps have disconnected areas that break the choke-point key/chest logic.
    if (_data->currentDepth % 5 != 0 && isBSP) {
        // Find stairs to ensure they are always reachable
        int sx = -1, sy = -1, totalFloors = 0;
        for (int y = 0; y < RogueSharedData::MAP_H; y++) {
            for (int x = 0; x < RogueSharedData::MAP_W; x++) {
                if (_data->map[y][x] == TileType::STAIRS_DOWN) { sx = x; sy = y; }
                if (_data->map[y][x] != TileType::WALL) totalFloors++;
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

    // Apply Flooded and Overgrown mutators
    if (_data->currentMutator == LevelMutator::FLOODED || _data->currentMutator == LevelMutator::OVERGROWN) {
        TileType targetTile = (_data->currentMutator == LevelMutator::FLOODED) ? TileType::WATER : TileType::TALL_GRASS;
        int chance = (_data->currentMutator == LevelMutator::FLOODED) ? 35 : 45;
        
        for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
            for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
                if (_data->map[y][x] == TileType::FLOOR && (x != _data->player.x || y != _data->player.y)) {
                    if (random(100) < chance) {
                        _data->map[y][x] = targetTile;
                    }
                }
            }
        }
    }

    spawnMonsters(_data);

    // Display Level Entry Message
    if (_data->currentMutator == LevelMutator::PITCH_BLACK) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Pitch Black...");
    } else if (_data->currentMutator == LevelMutator::INFESTED) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Infested!");
    } else if (_data->currentMutator == LevelMutator::TREASURE_TROVE) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Treasure Trove!");
    } else if (_data->currentMutator == LevelMutator::FLOODED) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Flooded!");
    } else if (_data->currentMutator == LevelMutator::OVERGROWN) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Overgrown!");
    } else if (_data->currentMutator == LevelMutator::LABYRINTH) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Labyrinth!");
    } else {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Floor %d", _data->currentDepth);
    }
    _data->hudMessageTimer = 80;
}

void generateBSPMap(RogueSharedData* _data) {
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
            int chestChance = (_data->currentMutator == LevelMutator::TREASURE_TROVE) ? 60 : 30;
            if (random(100) < chestChance) {
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

    // Spawn Altar (30% chance per floor)
    if (random(100) < 30) {
        int ax, ay;
        do {
            ax = random(1, RogueSharedData::MAP_W - 1);
            ay = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[ay][ax] != TileType::FLOOR || (ax == _data->player.x && ay == _data->player.y));
        _data->map[ay][ax] = TileType::ALTAR;
    }

    // Spawn Spikes
    int numSpikes = random(2, 7);
    if (_data->currentMutator == LevelMutator::TREASURE_TROVE) numSpikes *= 2;
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

    // Spawn Rubble and Webs
    for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
        for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
            if (_data->map[y][x] == TileType::FLOOR) {
                if (random(100) < 4) _data->map[y][x] = TileType::RUBBLE;
                if (random(100) < 3) _data->map[y][x] = TileType::WEB;
            }
        }
    }
}

void generateCaveMap(RogueSharedData* _data) {
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
    if (_data->currentMutator == LevelMutator::TREASURE_TROVE) numChests += random(2, 4);
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

    // Spawn Altar (30% chance per floor)
    if (random(100) < 30) {
        int ax, ay;
        do {
            ax = random(1, RogueSharedData::MAP_W - 1);
            ay = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[ay][ax] != TileType::FLOOR || (ax == _data->player.x && ay == _data->player.y));
        _data->map[ay][ax] = TileType::ALTAR;
    }

    // Spawn Spikes
    int numSpikes = random(2, 7);
    if (_data->currentMutator == LevelMutator::TREASURE_TROVE) numSpikes *= 2;
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

    // Spawn Water (Clusters) and Webs
    for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
        for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
            if (_data->map[y][x] == TileType::FLOOR) {
                if (_data->currentBiome == Biome::SEWERS && random(100) < 15) {
                    _data->map[y][x] = TileType::WATER;
                } else if (random(100) < 5) {
                    _data->map[y][x] = TileType::WEB;
                }
            }
        }
    }
}

void generateMazeMap(RogueSharedData* _data) {
    // Fill completely with walls
    for (int y = 0; y < RogueSharedData::MAP_H; y++) {
        for (int x = 0; x < RogueSharedData::MAP_W; x++) {
            _data->map[y][x] = TileType::WALL;
        }
    }

    int curX = RogueSharedData::MAP_W / 2;
    int curY = RogueSharedData::MAP_H / 2;
    int floorCount = 0;
    int targetFloors = (RogueSharedData::MAP_W * RogueSharedData::MAP_H) * 0.35f; // Carve 35% of the map
    
    _data->player.x = curX;
    _data->player.y = curY;
    
    // Tunnel Digger
    while(floorCount < targetFloors) {
        if (_data->map[curY][curX] == TileType::WALL) {
            _data->map[curY][curX] = TileType::FLOOR;
            floorCount++;
        }
        
        int dir = random(4);
        int nx = curX + (dir == 0 ? 1 : (dir == 1 ? -1 : 0));
        int ny = curY + (dir == 2 ? 1 : (dir == 3 ? -1 : 0));
        
        // Keep in bounds with 1 tile padding
        if (nx > 0 && nx < RogueSharedData::MAP_W - 1 && ny > 0 && ny < RogueSharedData::MAP_H - 1) {
            curX = nx; 
            curY = ny;
        }
    }
    
    // Last position becomes the stairs? NO, use a distance check!
    int sx, sy, attempts = 0;
    do {
        sx = random(1, RogueSharedData::MAP_W - 1);
        sy = random(1, RogueSharedData::MAP_H - 1);
        attempts++;
    } while ((_data->map[sy][sx] != TileType::FLOOR || (abs(sx - _data->player.x) + abs(sy - _data->player.y) < 15)) && attempts < 500);
    _data->map[sy][sx] = TileType::STAIRS_DOWN;

    // Spawn 1 Merchant
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

    // Scatter Chests
    int numChests = random(1, 4);
    if (_data->currentMutator == LevelMutator::TREASURE_TROVE) numChests += random(2, 4);
    for (int i = 0; i < numChests; i++) {
        int cx, cy;
        do {
            cx = random(1, RogueSharedData::MAP_W - 1);
            cy = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[cy][cx] != TileType::FLOOR || (cx == _data->player.x && cy == _data->player.y));
        _data->map[cy][cx] = TileType::CHEST;
    }

    // Spawn Altar (30% chance per floor)
    if (random(100) < 30) {
        int ax, ay;
        do {
            ax = random(1, RogueSharedData::MAP_W - 1);
            ay = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[ay][ax] != TileType::FLOOR || (ax == _data->player.x && ay == _data->player.y));
        _data->map[ay][ax] = TileType::ALTAR;
    }

    // Spawn Spikes
    int numSpikes = random(2, 7);
    if (_data->currentMutator == LevelMutator::TREASURE_TROVE) numSpikes *= 2;
    for (int i = 0; i < numSpikes; i++) {
        int spx, spy;
        do {
            spx = random(1, RogueSharedData::MAP_W - 1);
            spy = random(1, RogueSharedData::MAP_H - 1);
        } while (_data->map[spy][spx] != TileType::FLOOR || (spx == _data->player.x && spy == _data->player.y));
        _data->map[spy][spx] = TileType::SPIKE;
    }

    // Spawn Tall Grass, Rubble and Webs
    for (int y = 1; y < RogueSharedData::MAP_H - 1; y++) {
        for (int x = 1; x < RogueSharedData::MAP_W - 1; x++) {
            if (_data->map[y][x] == TileType::FLOOR) {
                if (random(100) < 15) _data->map[y][x] = TileType::TALL_GRASS;
                else if (random(100) < 4) _data->map[y][x] = TileType::RUBBLE;
                else if (random(100) < 3) _data->map[y][x] = TileType::WEB;
            }
        }
    }
}

void generateBossMap(RogueSharedData* _data) {
    for (int y = 0; y < RogueSharedData::MAP_H; y++) {
        for (int x = 0; x < RogueSharedData::MAP_W; x++) {
            _data->map[y][x] = TileType::WALL;
        }
    }

    int variant = random(3); // 0 = Open, 1 = Pillars, 2 = Moat

    for (int y = 8; y <= 24; y++) {
        for (int x = 8; x <= 24; x++) {
            if (x == 8 || x == 24 || y == 8 || y == 24) {
                _data->map[y][x] = TileType::WALL;
            } else {
                _data->map[y][x] = TileType::FLOOR;
                
                // Variant 1: Pillars for cover
                if (variant == 1 && (x % 4 == 0) && (y % 4 == 0)) {
                    _data->map[y][x] = TileType::WALL;
                }
                // Variant 2: Moat around the center
                else if (variant == 2 && (x >= 12 && x <= 20) && (y >= 12 && y <= 20) && (x == 12 || x == 20 || y == 12 || y == 20)) {
                    // Leave bridges at the exact midpoints
                    if (x != 16 && y != 16) {
                        _data->map[y][x] = TileType::WATER;
                    }
                }
            }
        }
    }
    _data->player.x = 16;
    _data->player.y = 22;
}

void spawnMonsters(RogueSharedData* _data) {
    for (auto& m : _data->monsters) m.active = false;

    float depth = (float)_data->currentDepth;

    if (_data->currentDepth % 5 == 0) {
        // Boss Room Setup
        _data->monsters[0].active = true;
        _data->monsters[0].type = MonsterType::BOSS;
        _data->monsters[0].x = 16;
        _data->monsters[0].y = 10;
        _data->monsters[0].maxHp = 40 + (int)(depth * 8.0f);
        _data->monsters[0].hp = _data->monsters[0].maxHp;
        _data->monsters[0].attack = 4 + (int)(depth * 1.5f);
        _data->monsters[0].defense = 2 + (int)(depth * 0.8f);
        _data->monsters[0].alert = true;
        return;
    }

    int targetMonsters = (_data->currentDepth * 2) + 6;
    if (_data->currentMutator == LevelMutator::INFESTED) targetMonsters *= 2;
    
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
        
        _data->monsters[i].maxHp = 6 + (int)(depth * 2.5f);
        _data->monsters[i].attack = 2 + (int)(depth * 0.8f);
        _data->monsters[i].defense = (int)(depth * 0.4f);

        if (_data->currentMutator == LevelMutator::INFESTED) {
            _data->monsters[i].type = (random(2) == 0) ? MonsterType::RAT : MonsterType::BAT;
            _data->monsters[i].maxHp = (_data->monsters[i].maxHp * 80) / 100; // -20% HP for swarms
        } else if (_data->currentDepth < 5) {
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
                _data->monsters[i].maxHp *= 2;
                _data->monsters[i].attack += 2;
            }
        }
        _data->monsters[i].hp = _data->monsters[i].maxHp;
        _data->monsters[i].alert = false;
    }
}

}
