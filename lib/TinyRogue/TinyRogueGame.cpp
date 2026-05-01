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
    
    // Hard exit back to OS
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }

    // Start Game
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.replace(_play, ctx);
    }
}

void RogueTitleScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_7x13B_tf);
    ctx.drawStr(32, 24, "TINY ROGUE");
    ctx.drawHLine(0, 30, Console::W);

    // Blinking prompt
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

void RoguePlayScene::_generateMap() {
    // 1. Fill map with solid walls
    for (int y = 0; y < RogueSharedData::MAP_H; y++) {
        for (int x = 0; x < RogueSharedData::MAP_W; x++) {
            _data->map[y][x] = TileType::WALL;
        }
    }

    // 2. BSP Tree Allocation (Max depth 4 = 31 nodes)
    const int MAX_NODES = 31;
    BSPNode nodes[MAX_NODES];
    int numNodes = 0;

    // Create root node representing the entire map bounds
    nodes[numNodes++] = { {1, 1, RogueSharedData::MAP_W - 2, RogueSharedData::MAP_H - 2}, {0,0,0,0}, -1, -1 };

    // 3. Subdivide the space
    for (int i = 0; i < numNodes; i++) {
        if (numNodes >= MAX_NODES - 1) break; // Reached maximum tree depth

        Rect b = nodes[i].bounds;
        
        // Determine split direction based on aspect ratio to prevent overly skinny areas
        bool splitH = random(2) == 0;
        if (b.w > b.h && b.w / b.h >= 1.25f) splitH = false; // Force vertical cut
        else if (b.h > b.w && b.h / b.w >= 1.25f) splitH = true;  // Force horizontal cut

        int maxSplit = (splitH ? b.h : b.w) - 6; // Leave minimum size of 6 units
        if (maxSplit <= 6) continue; // Area is too small to split further

        int splitLoc = random(6, maxSplit);

        // Assign child indices
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

    // 4. Carve Rooms in the Leaves
    int leafCount = 0;
    int leafIndices[16];

    for (int i = 0; i < numNodes; i++) {
        // If the node has no children, it's a leaf
        if (nodes[i].leftNode == -1 && nodes[i].rightNode == -1) { 
            Rect b = nodes[i].bounds;
            
            // Random room size within the partitioned bounds
            int rw = random(4, b.w - 1);
            int rh = random(4, b.h - 1);
            int rx = b.x + random(1, b.w - rw);
            int ry = b.y + random(1, b.h - rh);

            nodes[i].room = {rx, ry, rw, rh};

            // Carve floor
            for (int y = ry; y < ry + rh; y++) {
                for (int x = rx; x < rx + rw; x++) {
                    _data->map[y][x] = TileType::FLOOR;
                }
            }

            if (leafCount < 16) leafIndices[leafCount++] = i;
        }
    }

    // 5. Connect Rooms (Bottom-Up Traversal)
    for (int i = numNodes - 1; i >= 0; i--) {
        if (nodes[i].leftNode != -1) {
            BSPNode& l = nodes[nodes[i].leftNode];
            BSPNode& r = nodes[nodes[i].rightNode];

            // Parent inherits the left child's room so nodes further up the tree can connect to it
            nodes[i].room = l.room; 

            // Connect the center of the left subtree's room to the right subtree's room
            int lx = l.room.x + l.room.w / 2;
            int ly = l.room.y + l.room.h / 2;
            int rx = r.room.x + r.room.w / 2;
            int ry = r.room.y + r.room.h / 2;

            int curX = lx, curY = ly;
            
            // Carve L-shaped corridor. Only overwrite WALL to preserve existing floors
            if (random(2) == 0) {
                while (curX != rx) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(rx - curX); }
                while (curY != ry) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(ry - curY); }
            } else {
                while (curY != ry) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(ry - curY); }
                while (curX != rx) { if (_data->map[curY][curX] == TileType::WALL) _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(rx - curX); }
            }
        }
    }

    // 6. Spawn Player, Exits, and Loot
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

    // 7. Spawn Monsters
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
        _data->monsters[i].type = (random(3) == 0) ? MonsterType::GOBLIN : MonsterType::RAT;
    }
    
    // Note: ensure you use the `true` parameter here if you added smooth camera tracking
    _updateCamera(true); 
}

void RoguePlayScene::_updateCamera(bool snap) {
    // Calculate target pixel position to center the player
    int targetX = (_data->player.x * 8) - (Console::W / 2) + 4;
    int targetY = (_data->player.y * 8) - (Console::H / 2) + 4;

    // Clamp to map boundaries (in pixels)
    targetX = gclamp(targetX, 0, (RogueSharedData::MAP_W * 8) - Console::W);
    targetY = gclamp(targetY, 0, (RogueSharedData::MAP_H * 8) - Console::H);

    if (snap) {
        _camPixelX = _camStartX = _camTargetX = targetX;
        _camPixelY = _camStartY = _camTargetY = targetY;
        _camT = CAM_FRAMES;
    } else if (targetX != _camTargetX || targetY != _camTargetY) {
        // Player moved, start a new lerp from the current visual position
        _camStartX = _camPixelX;
        _camStartY = _camPixelY;
        _camTargetX = targetX;
        _camTargetY = targetY;
        _camT = 0;
    }

    // Advance interpolation every frame
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
    if (ctx.justPressed(Btn::UP))    dy = -1;
    if (ctx.justPressed(Btn::DOWN))  dy = 1;
    if (ctx.justPressed(Btn::LEFT))  dx = -1;
    if (ctx.justPressed(Btn::RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        _processTurn(dx, dy);         
        _processMonsterTurns(ctx, sm); 
    }
    
    // Call every frame to process the lerp animation
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

        // Simple Line-of-Sight/Aggro radius check (e.g., 6 tiles)
        int dx = _data->player.x - m.x;
        int dy = _data->player.y - m.y;
        
        if (abs(dx) <= 6 && abs(dy) <= 6) {
            // Determine step direction using engine helper
            int stepX = gsign(dx);
            int stepY = gsign(dy);

            // Favor horizontal or vertical movement randomly to avoid perfect diagonal tracking
            if (stepX != 0 && stepY != 0) {
                if (random(2) == 0) stepY = 0; 
                else stepX = 0;
            }

            int nx = m.x + stepX;
            int ny = m.y + stepY;

            // Attack player
            if (nx == _data->player.x && ny == _data->player.y) {
                _data->player.hp -= m.attack;
                playerHit = true;
            } 
            // Move into open space
            else if (_data->map[ny][nx] != TileType::WALL && !_getMonsterAt(nx, ny)) {
                m.x = nx;
                m.y = ny;
            }
        }
    }

    if (playerHit) {
        ctx.sfxDeath(); // Use standard hit/damage sound
        _shakeFrames = 6;
        
        if (_data->player.hp <= 0) {
            ctx.saveHiScore(_data->currentDepth); // Save depth as high score
            sm.replace(_dead, ctx);
        }
    } else {
        ctx.beep(400, 10); // Standard footstep sound
    }
}

void RoguePlayScene::_processTurn(int dx, int dy) {
    int targetX = _data->player.x + dx;
    int targetY = _data->player.y + dy;

    if (targetX < 0 || targetX >= RogueSharedData::MAP_W || targetY < 0 || targetY >= RogueSharedData::MAP_H) return;

    TileType targetTile = _data->map[targetY][targetX];
    if (targetTile == TileType::WALL) return; 

    // --- Bump Combat & Interactions ---
    Monster* targetMonster = _getMonsterAt(targetX, targetY);
    
    if (targetMonster) {
        targetMonster->hp -= _data->player.attack;
        
        if (targetMonster->hp <= 0) {
            targetMonster->active = false;
            
            // Gain XP and Level Up!
            _data->player.xp += targetMonster->maxHp;
            int xpNeeded = _data->player.level * 10;
            
            if (_data->player.xp >= xpNeeded) {
                _data->player.xp -= xpNeeded;
                _data->player.level++;
                _data->player.maxHp += 5;
                _data->player.hp = _data->player.maxHp; // Full heal on level up
                _data->player.attack += 1;
            }
        }
    } 
    else if (targetTile == TileType::CHEST) {
        // Open the chest!
        _data->map[targetY][targetX] = TileType::FLOOR; // Remove chest from map
        
        if (random(2) == 0) { // 50% chance for a health potion
            _data->player.hp = min(_data->player.maxHp, _data->player.hp + 5);
        } else {              // 50% chance for gold
            _data->gold += 15 * _data->currentDepth;
        }
    } 
    else {
        // --- Normal Movement ---
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
    
    // Draw minimalist HUD
    ctx.setFont(u8g2_font_5x7_tf);
    
    // Top Bar: Health and Level
    char topBuf[32];
    snprintf(topBuf, sizeof(topBuf), "HP:%d/%d  LVL:%d", _data->player.hp, _data->player.maxHp, _data->player.level);
    ctx.drawStr(2, 7, topBuf);

    // Bottom Bar: Depth and Gold
    char botBuf[32];
    snprintf(botBuf, sizeof(botBuf), "D:%d  G:%d", (unsigned)_data->currentDepth, (unsigned)_data->gold);
    int botWidth = ctx.strWidth(botBuf);
    ctx.drawStr(Console::W - botWidth - 2, Console::H - 2, botBuf);
}

void RoguePlayScene::drawDungeon(Console& ctx, int ox, int oy) const {
    // Add +1 to columns and rows to cover tiles partially scrolling off-screen
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

            // Apply pixel-perfect camera offset
            int renderX = (x * 8) + offsetX + ox;
            int renderY = (y * 8) + offsetY + oy; 

            TileType t = _data->map[mapY][mapX];

            // 1. Draw Base Tiles
            if (t == TileType::WALL) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_wall);
            } else if (t == TileType::FLOOR || t == TileType::CORRIDOR) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_floor);
            } else if (t == TileType::STAIRS_DOWN) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_stairs);
            } else if (t == TileType::CHEST) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_chest);
            }

            // 2. Draw Entities (using the new background masking from the tileset update)
            Monster* m = _getMonsterAt(mapX, mapY);
            if (m) {
                ctx.setDrawColor(0);
                ctx.drawBox(renderX, renderY, 8, 8);
                ctx.setDrawColor(1);
                
                if (m->type == MonsterType::RAT) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_rat);
                else if (m->type == MonsterType::GOBLIN) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_goblin);
            } 
            else if (mapX == _data->player.x && mapY == _data->player.y) {
                ctx.setDrawColor(0);
                ctx.drawBox(renderX, renderY, 8, 8);
                ctx.setDrawColor(1);
                
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_player);
            }
        }
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RoguePauseScene & RogueDeadScene (Boilerplate)
// ═════════════════════════════════════════════════════════════════════════════

void RoguePauseScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
        ctx.sfxMenuNav();
        sm.pop(ctx);
    }
}

void RoguePauseScene::draw(Console& ctx) {
    _play->drawDungeon(ctx, 0, 0); // Explicitly pass 0 offsets
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
    _play->drawDungeon(ctx, 0, 0); // Explicitly pass 0 offsets
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

    // Wire up the scene stack routing
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