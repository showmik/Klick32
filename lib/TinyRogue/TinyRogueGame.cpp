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
    uint8_t spawned = 0;
    for (auto& b : _blood) {
        if (b.life == 0) {
            b.x = px; b.y = py;
            b.vx = (random(-20, 21) / 10.0f);
            b.vy = (random(-20, 21) / 10.0f);
            b.life = random(5, 12);
            if (++spawned > 4) break;
        }
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
    do { 
        s = getOpenTile(); 
    } while (abs(s.ix() - _data->player.x) + abs(s.iy() - _data->player.y) < 15); // Ensure stairs aren't right next to player
    _data->map[s.iy()][s.ix()] = TileType::STAIRS_DOWN;

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

    int numMonsters = min((int)RogueSharedData::MAX_MONSTERS, (int)(_data->currentDepth + 2));
    for (int i = 0; i < numMonsters; i++) {
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
        _camPixelX = _camStartX = _camTargetX = targetX;
        _camPixelY = _camStartY = _camTargetY = targetY;
        _camT = CAM_FRAMES;
    } else if (targetX != _camTargetX || targetY != _camTargetY) {
        _camStartX = _camPixelX;
        _camStartY = _camPixelY;
        _camTargetX = targetX;
        _camTargetY = targetY;
        _camT = 0;
    }

    if (_camT < CAM_FRAMES) {
        _camT++;
        _camPixelX = lerpi(_camStartX, _camTargetX, _camT, CAM_FRAMES);
        _camPixelY = lerpi(_camStartY, _camTargetY, _camT, CAM_FRAMES);
    }
}

void RoguePlayScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.push(_pause, ctx);
        return;
    }

    if (_shakeFrames > 0) _shakeFrames--; 

    int dx = 0, dy = 0;
    
    if (ctx.repeat(Btn::UP))    dy = -1;
    if (ctx.repeat(Btn::DOWN))  dy = 1;
    if (ctx.repeat(Btn::LEFT))  dx = -1;
    if (ctx.repeat(Btn::RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        _processTurn(ctx, dx, dy);         
        _processMonsterTurns(ctx, sm); 
    }
    
    // Update combat particles
    for (auto& b : _blood) {
        if (b.life > 0) {
            b.x += b.vx; b.y += b.vy; b.life--;
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
        _shakeFrames = 6;
        
        if (_data->player.hp <= 0) {
            ctx.saveHiScore(_data->currentDepth); 
            sm.replace(_dead, ctx);
        }
    } else {
        ctx.beep(400, 10); 
    }
}

void RoguePlayScene::_processTurn(Console& ctx, int dx, int dy) {
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
        _shakeFrames = crit ? 6 : 3; 
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
            _shakeFrames = 8;
            
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
    else {
        _data->player.x = targetX;
        _data->player.y = targetY;

        if (targetTile == TileType::STAIRS_DOWN) {
            _data->currentDepth++;
            _generateMap(); 
        }
    }
}

void RoguePlayScene::draw(Console& ctx) {
    int ox = (_shakeFrames > 0) ? random(-2, 3) : 0;
    int oy = (_shakeFrames > 0) ? random(-2, 3) : 0;
    
    drawDungeon(ctx, ox, oy); 
    
    ctx.setFont(u8g2_font_5x7_tf);
    
    if (_hudMessageTimer > 0) {
        _hudMessageTimer--;
        int w = ctx.strWidth(_hudMessage);
        ctx.setDrawColor(0);
        ctx.drawBox((Console::W - w) / 2 - 2, 0, w + 4, 9);
        ctx.setDrawColor(1);
        ctx.drawStr((Console::W - w) / 2, 7, _hudMessage);
    } else {
        char topBuf[32];
        snprintf(topBuf, sizeof(topBuf), "HP:%d/%d L:%d", _data->player.hp, _data->player.maxHp, _data->player.level);
        
        ctx.setDrawColor(0);
        ctx.drawBox(0, 0, ctx.strWidth(topBuf) + 2, 9);
        ctx.setDrawColor(1);
        ctx.drawStr(1, 7, topBuf);

        char statBuf[32];
        snprintf(statBuf, sizeof(statBuf), "A:%d D:%d", _data->player.attack, _data->player.defense);
        int statW = ctx.strWidth(statBuf);
        
        ctx.setDrawColor(0);
        ctx.drawBox(Console::W - statW - 2, 0, statW + 2, 9);
        ctx.setDrawColor(1);
        ctx.drawStr(Console::W - statW - 1, 7, statBuf);
    }

    char botBuf[32];
    snprintf(botBuf, sizeof(botBuf), "D:%d  G:%d", (unsigned)_data->currentDepth, (unsigned)_data->gold);
    int botWidth = ctx.strWidth(botBuf);
    
    ctx.setDrawColor(0);
    ctx.drawBox(Console::W - botWidth - 3, Console::H - 9, botWidth + 3, 9);
    ctx.setDrawColor(1);
    ctx.drawStr(Console::W - botWidth - 1, Console::H - 2, botBuf);
}

void RoguePlayScene::drawDungeon(Console& ctx, int ox, int oy) const {
    int viewportTilesX = (Console::W / 8) + 1;
    int viewportTilesY = (Console::H / 8) + 1;
    
    int startCol = _camPixelX / 8;
    int startRow = _camPixelY / 8;
    int offsetX = -(_camPixelX % 8);
    int offsetY = -(_camPixelY % 8);

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

            int renderX = (x * 8) + offsetX + ox;
            int renderY = (y * 8) + offsetY + oy; 

            TileType t = _data->map[mapY][mapX];

            if (t == TileType::WALL) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_wall);
            } else if (t == TileType::FLOOR || t == TileType::CORRIDOR) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_floor);
            } else if (t == TileType::STAIRS_DOWN) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_stairs);
            } else if (t == TileType::CHEST) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_chest);
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

    // Draw Combat Particles over everything
    for (const auto& b : _blood) {
        if (b.life > 0) {
            int px = (int)b.x - _camPixelX + (Console::W / 2) - 4 + ox;
            int py = (int)b.y - _camPixelY + (Console::H / 2) - 4 + oy;
            ctx.drawPixel(px, py);
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RoguePauseScene & RogueDeadScene 
// ═════════════════════════════════════════════════════════════════════════════

void RoguePauseScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.pop(ctx);
    }
}

void RoguePauseScene::draw(Console& ctx) {
    _play->drawDungeon(ctx, 0, 0); 
    ctx.setDrawColor(0);
    ctx.drawBox(34, 22, 60, 22);
    ctx.setDrawColor(1);
    ctx.drawFrame(34, 22, 60, 22);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(42, 37, "PAUSED");
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
    ctx.setDrawColor(0);
    ctx.drawBox(20, 20, 88, 28);
    ctx.setDrawColor(1);
    ctx.drawFrame(20, 20, 88, 28);
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(34, 36, "YOU DIED");
    if ((_frame / 15) % 2 == 0) {
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(32, 45, "A to restart");
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
    _pause.setPlayScene(&_play);
    _dead.setData(&_data);
    _dead.setPlayScene(&_play);

    _sm.replace(&_title, ctx);
}

void TinyRogueGame::onExit(Console& ctx) {
    ctx.saveHiScore(_data.hiScore);
}

void TinyRogueGame::update(Console& ctx) { _sm.update(ctx); }
void TinyRogueGame::draw(Console& ctx)   { _sm.draw(ctx); }

bool        TinyRogueGame::isRunning() const { return !_sm.empty(); }
const char* TinyRogueGame::getName()   const { return "Tiny Rogue"; }