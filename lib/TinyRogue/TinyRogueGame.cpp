#include "TinyRogueGame.h"

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

    // 2. Simple Room Generation
    const int numRooms = 5;
    int prevCenterX = 0, prevCenterY = 0;

    for (int i = 0; i < numRooms; i++) {
        // Random room dimensions and position
        int rw = random(4, 8);
        int rh = random(4, 8);
        int rx = random(1, RogueSharedData::MAP_W - rw - 1);
        int ry = random(1, RogueSharedData::MAP_H - rh - 1);

        // Carve the room
        for (int y = ry; y < ry + rh; y++) {
            for (int x = rx; x < rx + rw; x++) {
                _data->map[y][x] = TileType::FLOOR;
            }
        }

        int centerX = rx + (rw / 2);
        int centerY = ry + (rh / 2);

        if (i == 0) {
            // Place player in the first room
            _data->player.x = centerX;
            _data->player.y = centerY;
        } else {
            // Carve L-shaped corridor to the previous room
            int curX = prevCenterX;
            int curY = prevCenterY;
            
            // Move horizontally first, then vertically (or vice versa randomly)
            if (random(2) == 0) {
                while (curX != centerX) { _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(centerX - curX); }
                while (curY != centerY) { _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(centerY - curY); }
            } else {
                while (curY != centerY) { _data->map[curY][curX] = TileType::CORRIDOR; curY += gsign(centerY - curY); }
                while (curX != centerX) { _data->map[curY][curX] = TileType::CORRIDOR; curX += gsign(centerX - curX); }
            }
        }

        if (i == numRooms - 1) {
            // Place exit stairs in the last room
            _data->map[centerY][centerX] = TileType::STAIRS_DOWN;
        }

        // 30% chance to spawn a chest in the corner of a room
        if (i > 0 && random(100) < 30) {
            _data->map[ry + 1][rx + 1] = TileType::CHEST;
        }

        prevCenterX = centerX;
        prevCenterY = centerY;
    }

    // --- Monster Spawning ---
    // Reset all monsters
    for (auto& m : _data->monsters) m.active = false;

    // Spawn more monsters as you go deeper (cap at MAX_MONSTERS)
    int numMonsters = min((int)RogueSharedData::MAX_MONSTERS, (int)(_data->currentDepth + 2));
    
    for (int i = 0; i < numMonsters; i++) {
        int mx, my;
        bool validSpot = false;
        
        // Find a random floor tile that isn't the player's starting spot
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
        
        // Scale health and damage with depth
        _data->monsters[i].hp = 4 + _data->currentDepth;
        _data->monsters[i].attack = 1 + (_data->currentDepth / 3);
        _data->monsters[i].type = (random(3) == 0) ? MonsterType::GOBLIN : MonsterType::RAT;
    }
    
    _updateCamera();
}

void RoguePlayScene::_updateCamera() {
    // Center the camera on the player. 
    // Console is 128x64. At 8x8 pixels per tile, viewport is 16x8 tiles.
    int viewportTilesX = Console::W / 8;
    int viewportTilesY = Console::H / 8;

    _camX = _data->player.x - (viewportTilesX / 2);
    _camY = _data->player.y - (viewportTilesY / 2);

    // Clamp camera so it doesn't show outside the map bounds
    _camX = gclamp(_camX, 0, RogueSharedData::MAP_W - viewportTilesX);
    _camY = gclamp(_camY, 0, RogueSharedData::MAP_H - viewportTilesY);
}

void RoguePlayScene::update(Console& ctx, SceneManager& sm) {
    if (ctx.justPressed(Btn::MENU1)) { sm.clear(ctx); return; }
    if (ctx.justPressed(Btn::MENU2) || ctx.justPressed(Btn::B)) {
        ctx.sfxMenuNav();
        sm.push(_pause, ctx);
        return;
    }

    if (_shakeFrames > 0) _shakeFrames--; // Decay screen shake

    int dx = 0, dy = 0;
    if (ctx.justPressed(Btn::UP))    dy = -1;
    if (ctx.justPressed(Btn::DOWN))  dy = 1;
    if (ctx.justPressed(Btn::LEFT))  dx = -1;
    if (ctx.justPressed(Btn::RIGHT)) dx = 1;

    if (dx != 0 || dy != 0) {
        _processTurn(dx, dy);         // 1. Player Acts
        _processMonsterTurns(ctx, sm); // 2. Monsters Act
        _updateCamera();              // 3. Update View
    }
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
    // We are rendering ASCII-style minimalist tiles using the 5x7 font
    ctx.setFont(u8g2_font_5x7_tf);
    
    int viewportTilesX = Console::W / 8;
    int viewportTilesY = Console::H / 8;

    for (int y = 0; y < viewportTilesY; y++) {
        for (int x = 0; x < viewportTilesX; x++) {
            int mapX = _camX + x;
            int mapY = _camY + y;

            if (mapX < 0 || mapX >= RogueSharedData::MAP_W || mapY < 0 || mapY >= RogueSharedData::MAP_H) continue;

            int screenX = x * 8;
            int screenY = (y * 8) + 8; // Offset by 8 because font rendering draws from the baseline

            TileType t = _data->map[mapY][mapX];

            // Draw Entities with shake offsets applied
            int renderX = screenX + ox;
            int renderY = screenY + oy;

            if (t == TileType::WALL) {
                ctx.drawStr(renderX, renderY, "#");
            } else if (t == TileType::FLOOR || t == TileType::CORRIDOR) {
                ctx.drawStr(renderX, renderY, ".");
            } else if (t == TileType::STAIRS_DOWN) {
                ctx.drawStr(renderX, renderY, ">");
            } else if (t == TileType::CHEST) {
                // Draw a chest symbol
                ctx.drawStr(renderX, renderY, "C");
            }

            // Draw Monsters
            Monster* m = _getMonsterAt(mapX, mapY);
            if (m) {
                if (m->type == MonsterType::RAT) ctx.drawStr(renderX, renderY, "r");
                else if (m->type == MonsterType::GOBLIN) ctx.drawStr(renderX, renderY, "g");
            } 
            // Draw Player
            else if (mapX == _data->player.x && mapY == _data->player.y) {
                ctx.drawStr(renderX, renderY, "@");
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