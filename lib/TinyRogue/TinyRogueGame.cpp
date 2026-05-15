#include "TinyRogueGame.h"
#include "TinyRogueSprites.h"

// ═════════════════════════════════════════════════════════════════════════════
// RogueTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void RogueTitleScene::onEnter(Console& ctx) {
    _frame = 0;
}

void RogueTitleScene::update(Console& ctx, SceneManager& sm) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);
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
    _data->currentDepth = 1;
    _data->gold = 0;
    _data->player.hp = _data->player.maxHp;
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

    // 2. Roll the dice for biome (70% Castle/BSP, 30% Cave/CA)
    if (random(100) < 70) {
        _generateBSPMap();
    } else {
        _generateCaveMap();
    }

    // 3. Shared Spawning & Setup
    _spawnMonsters();
    _updateCamera(true); 
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

    // Spawn exactly 1 Merchant on an open floor tile
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

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

    // Spawn exactly 1 Merchant on an open floor tile
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

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
}

void RoguePlayScene::_spawnMonsters() {
    for (auto& m : _data->monsters) m.active = false;

    // FIX: Significantly increase enemy density. 
    // Spawns 8 enemies on Floor 1, scaling up as you descend.
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
                validSpot = true;
            }
        }

        _data->monsters[i].x = mx;
        _data->monsters[i].y = my;
        _data->monsters[i].active = true;
        _data->monsters[i].hp = 4 + _data->currentDepth;
        _data->monsters[i].attack = 1 + (_data->currentDepth / 3);

        int r = random(4);
        if (r == 0)      _data->monsters[i].type = MonsterType::GOBLIN;
        else if (r == 1) _data->monsters[i].type = MonsterType::RAT;
        else if (r == 2) _data->monsters[i].type = MonsterType::BAT;
        else             _data->monsters[i].type = MonsterType::SKELETON;
    }
}

void RoguePlayScene::_updateCamera(bool snap) {
    int targetX = (_data->player.x * 8) - (Console::W / 2) + 4;
    int targetY = (_data->player.y * 8) - (Console::H / 2) + 4;

    targetX = gclamp(targetX, 0, (RogueSharedData::MAP_W * 8) - Console::W);
    targetY = gclamp(targetY, 0, (RogueSharedData::MAP_H * 8) - Console::H);

    if (snap) {
        _camera->snapTo(targetX, targetY);
    } else {
        _camera->panTo(targetX, targetY, 6);
    }
}

void RoguePlayScene::update(Console& ctx, SceneManager& sm) {
    if (_hudMessageTimer > 0) _hudMessageTimer--;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.push(_pause, ctx);
        return;
    }

    int dx = 0, dy = 0;
    if (ctx.justPressed(Btn::UP)   || ctx.repeat(Btn::UP))   dy = -1;
    if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) dy = 1;
    if (ctx.justPressed(Btn::LEFT) || ctx.repeat(Btn::LEFT)) dx = -1;
    if (ctx.justPressed(Btn::RIGHT)|| ctx.repeat(Btn::RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        _processTurn(ctx, sm, dx, dy);  // <--- Added 'sm' right here!      
        _processMonsterTurns(ctx, sm); 
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
        if (!m.active) continue;

        int aggro = 6;
        if (m.type == MonsterType::BAT) aggro = 8;
        else if (m.type == MonsterType::SKELETON) aggro = 5;

        if (m.type == MonsterType::SKELETON && (_data->turnCount % 2 != 0)) {
            continue; 
        }

        int steps = (m.type == MonsterType::BAT) ? 2 : 1;

        for (int s = 0; s < steps; s++) {
            int dx = _data->player.x - m.x;
            int dy = _data->player.y - m.y;

            if (abs(dx) <= aggro && abs(dy) <= aggro) {
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
    }

    if (playerHit) {
        ctx.sfxDeath(); 
        _camera->shake(6);
        
        if (_data->player.hp <= 0) {
            ctx.saveHiScore(_data->currentDepth); 
            sm.replace(_dead, ctx);
        }
    } else {
        ctx.beep(400, 10); 
    }
}

void RoguePlayScene::_processTurn(Console& ctx, SceneManager& sm, int dx, int dy) {
    _data->turnCount++; 

    int targetX = _data->player.x + dx;
    int targetY = _data->player.y + dy;

    if (targetX < 0 || targetX >= RogueSharedData::MAP_W || targetY < 0 || targetY >= RogueSharedData::MAP_H) return;

    TileType targetTile = _data->map[targetY][targetX];
    if (targetTile == TileType::WALL) return; 

    Monster* targetMonster = _getMonsterAt(targetX, targetY);
    
    if (targetMonster) {
        int dmg = _data->player.attack;
        bool crit = (random(100) < 20);
        if (crit) dmg *= 2;
        
        targetMonster->hp -= dmg;
        _camera->shake(crit ? 6 : 3); 
        _spawnHitEffect(targetX, targetY);
        ctx.beep(crit ? 1500 : 1000, 20); 
        
        if (targetMonster->hp <= 0) {
            targetMonster->active = false;
            _data->player.xp += targetMonster->maxHp;
            int xpNeeded = _data->player.level * 10;
            
            if (_data->player.xp >= xpNeeded) {
                _data->player.xp -= xpNeeded;
                _data->player.level++;
                _data->player.maxHp += 5;
                _data->player.hp = _data->player.maxHp; 
                _data->player.attack += 1;
                snprintf(_hudMessage, sizeof(_hudMessage), "LEVEL UP!");
                _hudMessageTimer = 60;
                ctx.beep(800, 100); ctx.beep(1200, 150);
            }
        }
    }
    else if (targetTile == TileType::CHEST) {
        _data->map[targetY][targetX] = TileType::FLOOR; 
        
        if (random(100) < 15) {
            snprintf(_hudMessage, sizeof(_hudMessage), "It's a MIMIC!");
            _hudMessageTimer = 60;
            ctx.beep(200, 150);
            _camera->shake(8);
            
            for (auto& m : _data->monsters) {
                if (!m.active) {
                    m.x = targetX; m.y = targetY;
                    m.active = true;
                    m.hp = 10 + (_data->currentDepth * 3);
                    m.attack = _data->player.attack + 1; 
                    m.type = MonsterType::GOBLIN; 
                    break;
                }
            }
            return; 
        }

        int roll = random(100);
        if (roll < 25) { 
            _data->player.attack += 1;
            snprintf(_hudMessage, sizeof(_hudMessage), "Found Sword! +1 ATK");
            _hudMessageTimer = 60; 
            ctx.beep(800, 40); ctx.beep(1200, 60); 
        } else if (roll < 50) { 
            _data->player.defense += 1;
            snprintf(_hudMessage, sizeof(_hudMessage), "Found Shield! +1 DEF");
            _hudMessageTimer = 60;
            ctx.beep(800, 40); ctx.beep(1200, 60);
        } else if (roll < 75) { 
            _data->player.maxHp += 5;
            _data->player.hp = _data->player.maxHp;
            snprintf(_hudMessage, sizeof(_hudMessage), "Elixir! Max HP Up");
            _hudMessageTimer = 60;
            ctx.beep(600, 40); ctx.beep(1000, 60);
        } else { 
            int amount = 15 * _data->currentDepth;
            _data->gold += amount;
            snprintf(_hudMessage, sizeof(_hudMessage), "Found %d Gold", amount);
            _hudMessageTimer = 60;
            ctx.beep(1200, 20); ctx.beep(1500, 40);
        }
    }
    else if (targetTile == TileType::MERCHANT) {
        // Push the shop overlay without ending the turn
        ctx.sfxMenuEnter();
        sm.push(_shop, ctx);
        return; 
    }
    else {
        _data->player.x = targetX;
        _data->player.y = targetY;

        if (targetTile == TileType::STAIRS_DOWN) {
            _data->currentDepth++;
            _generateMap(); 
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RogueShopScene 
// ═════════════════════════════════════════════════════════════════════════════

void RogueShopScene::onEnter(Console& ctx) { 
    _cursor = 0; 
    _msgTimer = 0;
    _introFrames = 0; // Reset animation
}

void RogueShopScene::update(Console& ctx, SceneManager& sm) {
    if (_introFrames < 10) _introFrames++;
    // Exit Shop
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuBack();
        sm.pop(ctx);
        return;
    }

    // Navigation
    if (ctx.justPressed(Btn::UP) || ctx.repeat(Btn::UP)) {
        if (_cursor > 0) { _cursor--; ctx.sfxMenuNav(); }
    }
    if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) {
        if (_cursor < 2) { _cursor++; ctx.sfxMenuNav(); }
    }

    if (_msgTimer > 0) _msgTimer--;

    // Purchasing Logic
    if (ctx.justPressed(Btn::A)) {
        int cost = (_cursor == 0) ? 20 : 50; // Health is 20g, upgrades are 50g

        if (_data->gold >= cost) {
            bool purchased = false;

            if (_cursor == 0 && _data->player.hp < _data->player.maxHp) {
                _data->player.hp = _data->player.maxHp;
                snprintf(_msg, sizeof(_msg), "Fully Healed!");
                purchased = true;
            } else if (_cursor == 0) {
                snprintf(_msg, sizeof(_msg), "Already full HP!");
            } else if (_cursor == 1) {
                _data->player.attack++;
                snprintf(_msg, sizeof(_msg), "Weapon Sharpened!");
                purchased = true;
            } else if (_cursor == 2) {
                _data->player.defense++;
                snprintf(_msg, sizeof(_msg), "Armor Fortified!");
                purchased = true;
            }

            if (purchased) {
                _data->gold -= cost;
                ctx.sfxPoint(); // Happy sound
            } else {
                ctx.beep(300, 30); // Error sound
            }
            _msgTimer = 60;

        } else {
            ctx.sfxDeath(); // Not enough money sound
            snprintf(_msg, sizeof(_msg), "Not enough Gold!");
            _msgTimer = 60;
        }
    }
}

void RogueShopScene::draw(Console& ctx) {
    _play->drawDungeon(ctx, 0, 0); 
    
    int yOff = lerpi(Console::H, 0, _introFrames, 10);
    int bx = 10, by = 8 + yOff, bw = 108, bh = 52; 

    // 1. Drop Shadow (Offset +2, +2)
    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);

    // 2. Main Box Background & Border
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);

    // 3. Inverted Header Anchor
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 14); 
    
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.setDrawColor(0); // Black text for negative space
    ctx.drawStr(bx + 4, by + 11, "MERCHANT");

    // Embed exit prompt in the right side of the header
    ctx.setFont(u8g2_font_5x7_tf);
    int escW = ctx.strWidth("[B] Exit");
    ctx.drawStr(bx + bw - escW - 4, by + 10, "[B] Exit"); 

    ctx.setDrawColor(1); // Reset back to white for the body
    
    char gBuf[16]; 
    snprintf(gBuf, sizeof(gBuf), "Wallet: %u g", _data->gold);
    ctx.drawStr(bx + 4, by + 24, gBuf);

    ctx.drawStr(bx + 14, by + 34, "Heal HP   (20g)");
    ctx.drawStr(bx + 14, by + 42, "Up ATK    (50g)");
    ctx.drawStr(bx + 14, by + 50, "Up DEF    (50g)");

    ctx.drawStr(bx + 4, by + 34 + (_cursor * 8), ">");

    // Dynamic Event Message Overlay
    if (_msgTimer > 0) {
        ctx.setDrawColor(0);
        ctx.drawBox(bx + 2, by + 36, bw - 4, 14);
        ctx.setDrawColor(1);
        
        int msgW = ctx.strWidth(_msg);
        ctx.drawStr(bx + (bw - msgW) / 2, by + 46, _msg);
    }
}

void RoguePlayScene::draw(Console& ctx) {
    drawDungeon(ctx, 0, 0); 
    
    // Top Center: HUD Messages
    ctx.setFont(u8g2_font_5x7_tf);
    
    // Top Center: HUD Messages
    if (_hudMessageTimer > 0) {
        // Because timer goes from 60 down to 0, lerpi mapping (0, 12) means
        // it starts at Y=12 and floats UP to Y=0 as the timer runs out.
        int floatY = lerpi(0, 12, _hudMessageTimer, 60);
        
        // 1-bit "Fade Out": Flicker rapidly during the last 15 frames
        bool visible = (_hudMessageTimer > 15) || (_hudMessageTimer % 2 == 0);
        
        if (visible) {
            int w = ctx.strWidth(_hudMessage);
            ctx.setDrawColor(0);
            ctx.drawBox((Console::W - w) / 2 - 2, floatY, w + 4, 9);
            ctx.setDrawColor(1);
            ctx.drawStr((Console::W - w) / 2, floatY + 7, _hudMessage);
        }
    } else {
        // Top Left: HP (Heart Icon) and Level
        char topBuf[32];
        snprintf(topBuf, sizeof(topBuf), "%d/%d L:%d", _data->player.hp, _data->player.maxHp, _data->player.level);
        int topW = ctx.strWidth(topBuf);
        
        ctx.setDrawColor(0);
        ctx.drawBox(0, 0, 10 + topW + 2, 10);
        ctx.setDrawColor(1);
        ctx.drawBitmap(1, 1, 1, 8, spr_icon_heart);
        ctx.drawStr(11, 7, topBuf);

        // Top Right: Attack (Sword) and Defense (Shield)
        char atkBuf[8], defBuf[8];
        snprintf(atkBuf, sizeof(atkBuf), "%d", _data->player.attack);
        snprintf(defBuf, sizeof(defBuf), "%d", _data->player.defense);
        
        int atkW = ctx.strWidth(atkBuf);
        int defW = ctx.strWidth(defBuf);
        // Calculate total width: [Sword] + ATK + space + [Shield] + DEF
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

    // Bottom Right: Depth (Stairs) and Gold (Coin)
    char depBuf[8], goldBuf[16];
    snprintf(depBuf, sizeof(depBuf), "%d", _data->currentDepth);
    snprintf(goldBuf, sizeof(goldBuf), "%d", _data->gold);
    
    int depW = ctx.strWidth(depBuf);
    int goldW = ctx.strWidth(goldBuf);
    // Calculate total width: [Stairs] + Depth + space + [Coin] + Gold
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

            // Line of Sight Math
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
            } else if (t == TileType::STAIRS_DOWN) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_stairs);
            } else if (t == TileType::CHEST) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_chest);
            } else if (t == TileType::MERCHANT) {            
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_merchant);
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
            } 
            else if (mapX == _data->player.x && mapY == _data->player.y) {
                ctx.setDrawColor(0);
                ctx.drawBox(renderX, renderY, 8, 8);
                ctx.setDrawColor(1);
                
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_player);
            }

            // Dither pattern for explored but currently unseen tiles
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
    ctx.setCamera(nullptr);
}

// ═════════════════════════════════════════════════════════════════════════════
// RoguePauseScene & RogueDeadScene 
// ═════════════════════════════════════════════════════════════════════════════

void RoguePauseScene::update(Console& ctx, SceneManager& sm) {
    if (_introFrames < 8) _introFrames++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.pop(ctx);
    }
}

void RoguePauseScene::draw(Console& ctx) {
    _play->drawDungeon(ctx, 0, 0); 
    
    int yOff = lerpi(-30, 0, _introFrames, 8);
    int bx = 30, by = 22 + yOff, bw = 68, bh = 24;

    // 1. Drop Shadow
    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);

    // 2. Main Box Background & Border
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);

    // 3. Inverted Header Anchor
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 14);

    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.setDrawColor(0);
    int tw = ctx.strWidth("PAUSED");
    ctx.drawStr(bx + (bw - tw) / 2, by + 11, "PAUSED");

    // Sub-text in the remaining black space
    ctx.setDrawColor(1);
    ctx.setFont(u8g2_font_5x7_tf);
    tw = ctx.strWidth("[B] Resume");
    ctx.drawStr(bx + (bw - tw) / 2, by + 21, "[B] Resume");
}

void RogueDeadScene::onEnter(Console& ctx) { _frame = 0; }

void RogueDeadScene::update(Console& ctx, SceneManager& sm) {
    _frame++;
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);
    }
}

void RogueDeadScene::draw(Console& ctx) {
    _play->drawDungeon(ctx, 0, 0); 
    
    int bx = 20, by = 20, bw = 88, bh = 28;

    // 1. Drop Shadow
    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh);

    // 2. Main Box Background & Border
    ctx.setDrawColor(0);
    ctx.drawBox(bx, by, bw, bh);
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);

    // 3. Inverted Header Anchor
    ctx.setDrawColor(1);
    ctx.drawBox(bx, by, bw, 14);

    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.setDrawColor(0);
    int tw = ctx.strWidth("YOU DIED");
    ctx.drawStr(bx + (bw - tw) / 2, by + 11, "YOU DIED");
    
    // Blinking prompt in the body
    ctx.setDrawColor(1);
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        tw = ctx.strWidth("A to restart");
        ctx.drawStr(bx + (bw - tw) / 2, by + 24, "A to restart");
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// TinyRogueGame - OS Registration Hook
// ═════════════════════════════════════════════════════════════════════════════

void TinyRogueGame::onEnter(Console& ctx) {
    _data.hiScore = ctx.loadHiScore();

    _title.setPlayScene(&_play);
    _play.setData(&_data);
    _play.setPauseScene(&_pause);
    _play.setDeadScene(&_dead);
    _play.setEngine(&_camera, &_particles);
    
    _shop.setData(&_data);      
    _shop.setPlayScene(&_play); 

    _pause.setPlayScene(&_play);
    _dead.setData(&_data);
    _dead.setPlayScene(&_play);

    _sm.replace(&_title, ctx);
}

void TinyRogueGame::onExit(Console& ctx) {
    ctx.saveHiScore(_data.hiScore);
}

void TinyRogueGame::update(Console& ctx) { 
    _camera.update();
    _particles.update();
    _sm.update(ctx); 
}
void TinyRogueGame::draw(Console& ctx)   { _sm.draw(ctx); }

bool        TinyRogueGame::isRunning() const { return !_sm.empty(); }
const char* TinyRogueGame::getName()   const { return "Tiny Rogue"; }
const uint8_t* TinyRogueGame::getCoverArt() const { return spr_tinyrogue_cover; }