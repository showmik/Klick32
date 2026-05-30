#include "TinyRogueMapGen.h"
#include "TinyRogueCombat.h"
#include "TinyRogueGame.h"
#include "GameRegistry.h"
#include "TinyRogueSprites.h"
#include "CommonScreens.h"


// ═════════════════════════════════════════════════════════════════════════════
// RogueTitleScene
// ═════════════════════════════════════════════════════════════════════════════

void RogueTitleScene::onEnter(Console& ctx) {
    _frame = 0;
    
    // Initialize bats
    for (int i = 0; i < 5; i++) {
        _bats[i].x = (float)random(0, Console::W);
        _bats[i].y = (float)random(0, Console::H / 2);
        _bats[i].vx = (float)(random(10, 20) / 10.0f) * (random(2) == 0 ? 1.0f : -1.0f);
        _bats[i].vy = (float)(random(5, 10) / 10.0f) * (random(2) == 0 ? 1.0f : -1.0f);
    }
    
    // Initialize sparks
    for (int i = 0; i < 15; i++) {
        _sparks[i].x = (float)random(0, Console::W);
        _sparks[i].y = (float)(Console::H + random(0, 30));
        _sparks[i].vy = -(float)(random(5, 15) / 10.0f);
        _sparks[i].life = (int)random(30, 80);
    }
}

void RogueTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    _frame++;

    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::MENU1)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Map CUSTOM_1 to PlayScene in Game
    }
    
    // Update bats
    for (int i = 0; i < 5; i++) {
        _bats[i].x += _bats[i].vx;
        _bats[i].y += _bats[i].vy;
        
        // Bounce bats
        if (_bats[i].x < 0) { _bats[i].x = 0; _bats[i].vx *= -1; }
        if (_bats[i].x > Console::W - 8) { _bats[i].x = Console::W - 8; _bats[i].vx *= -1; }
        if (_bats[i].y < 0) { _bats[i].y = 0; _bats[i].vy *= -1; }
        if (_bats[i].y > Console::H - 20) { _bats[i].y = Console::H - 20; _bats[i].vy *= -1; }
        
        // Randomly change direction occasionally
        if (random(100) < 2) _bats[i].vx *= -1;
        if (random(100) < 2) _bats[i].vy *= -1;
    }
    
    // Update sparks
    for (int i = 0; i < 15; i++) {
        _sparks[i].y += _sparks[i].vy;
        _sparks[i].x += sin(_frame * 0.1f + i) * 0.5f; // Sway
        _sparks[i].life--;
        
        if (_sparks[i].life <= 0 || _sparks[i].y < 0) {
            _sparks[i].x = (float)random(0, Console::W);
            _sparks[i].y = (float)(Console::H + random(0, 10));
            _sparks[i].vy = -(float)(random(5, 15) / 10.0f);
            _sparks[i].life = (int)random(30, 80);
        }
    }
}

void RogueTitleScene::draw(Console& ctx) {
    // Draw background elements
    ctx.setDrawColor(0);
    ctx.drawBox(0, 0, Console::W, Console::H);
    ctx.setDrawColor(1);
    
    // Draw floor patterns at the bottom
    for (int i = 0; i < Console::W; i += 8) {
        ctx.drawBitmap(i, Console::H - 8, 1, 8, spr_rogue_wall);
    }
    
    // Draw Sparks
    for (int i = 0; i < 15; i++) {
        if (_sparks[i].life > 0) {
            ctx.drawPixel((int)_sparks[i].x, (int)_sparks[i].y);
        }
    }
    
    // Draw Bats
    for (int i = 0; i < 5; i++) {
        uint8_t flip = (_bats[i].vx > 0) ? Console::BMP_FLIP_H : Console::BMP_FLIP_NONE;
        int yOff = ((_frame / 5 + i) % 2 == 0) ? 1 : 0; // Simple flap animation
        ctx.drawBitmapEx((int)_bats[i].x, (int)_bats[i].y + yOff, 1, 8, spr_rogue_bat, flip);
    }
    
    // Draw Title Text
    Screens::drawTitle(ctx, "TINY ROGUE");
    
    // Draw High Score
    if (_data && _data->hiScore > 0) {
        ctx.setFont(u8g2_font_4x6_tr);
        ctx.drawPrintfCentered(45, "HI-SCORE: %lu", (unsigned long)_data->hiScore);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// RoguePlayScene
// ═════════════════════════════════════════════════════════════════════════════

void RoguePlayScene::onEnter(Console& ctx) {
    _descending = false;
    _fadeTimer = 0;
    _altarMenuOpen = false;
    if (_resumed) {
        _resumed = false;
        TinyRogueCombat::recalcStats(_data);
        _updateCamera(true);
        isAiming = false;
        return;
    }

    _data->currentDepth = 1;
    _data->gold = 0;
    _data->registry.clear();
    _data->playerID = _data->registry.create();
    _data->players.add(_data->playerID, {0, 1, 0});
    _data->transforms.add(_data->playerID, {0, 0});
    _data->healths.add(_data->playerID, {10, 10});
    _data->combats.add(_data->playerID, {2, 0, 2, 0, 0, 0});
    _data->equippedWeapon.type = ItemType::DAGGER;
    _data->equippedWeapon.level = 0;
    _data->equippedWeapon.count = 1;
    _data->equippedArmor.type = ItemType::NONE;
    _data->equippedAccessory.type = ItemType::NONE;
    TinyRogueCombat::recalcStats(_data);
    for (int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
        _data->inventory[i].type = ItemType::NONE;
    }
    _data->inventory[0].type = ItemType::POTION;
    _data->inventory[0].count = 1;
    _data->inventory[0].level = 0;
    TinyRogueMapGen::generateMap(_data);
}















void RoguePlayScene::_updateCamera(bool snap) {
    int focusX = isAiming ? aimX : _data->transforms.data[_data->playerID].x;
    int focusY = isAiming ? aimY : _data->transforms.data[_data->playerID].y;

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

void RoguePlayScene::saveSnapshot(Console& ctx) {
    if (_data && _data->healths.data[_data->playerID].hp > 0) {
        ctx.saveBytes("gamestate", _data, sizeof(RogueSharedData));
    }
}

void RoguePlayScene::loadSnapshot(Console& ctx) {
    if (_data && ctx.hasSave("gamestate")) {
        ctx.loadBytes("gamestate", _data, sizeof(RogueSharedData));
    }
}

void RoguePlayScene::onSnapshotRestored(Console& ctx) {
    _descending = false;
    _fadeTimer = 0;
    _altarMenuOpen = false;
    isAiming = false;
    TinyRogueCombat::recalcStats(_data);
    _updateCamera(true);
}

void RoguePlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (_data->hudMessageTimer > 0) _data->hudMessageTimer--;

    // --- Fade Transition Logic ---
    if (_descending) {
        _fadeTimer--;
        if (_fadeTimer == 0) {
            // Screen is completely black, generate the next level!
            _data->currentDepth++;
            TinyRogueMapGen::generateMap(_data); 
            _camera->snapTo((_data->transforms.data[_data->playerID].x * 8) - (Console::W / 2) + 4, 
                            (_data->transforms.data[_data->playerID].y * 8) - (Console::H / 2) + 4);
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

    if (_altarMenuOpen) {
        if (ctx.justPressed(Btn::UP) || ctx.justPressed(Btn::DOWN)) {
            _altarMenuCursor = (_altarMenuCursor == 0) ? 1 : 0;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::B)  || ctx.justPressed(Btn::LEFT)) {
            _altarMenuOpen = false;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::A)) {
            if (_altarMenuCursor == 1) { // Leave
                _altarMenuOpen = false;
                ctx.sfxMenuNav();
            } else { // Sacrifice
                _altarMenuOpen = false;
                _data->healths.data[_data->playerID].hp -= 5;
                _camera->shake(6);
                ctx.sfxDeath();
                
                if (_data->healths.data[_data->playerID].hp <= 0) {
                    snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Fatal Sacrifice!");
                    _data->hudMessageTimer = 60;
                    if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
                    sm.emit(ctx, Event::GAME_OVER);
                    return;
                } else {
                    int boon = random(3);
                    if (boon == 0) {
                        _data->combats.data[_data->playerID].baseAttack += 1;
                        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boon: +1 Attack!");
                    } else if (boon == 1) {
                        _data->combats.data[_data->playerID].baseDefense += 1;
                        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boon: +1 Defense!");
                    } else {
                        bool added = false;
                        for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                            if(_data->inventory[i].type == ItemType::SCROLL_UPGRADE) {
                                _data->inventory[i].count++;
                                added = true; break;
                            }
                        }
                        if(!added) {
                            for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                                if(_data->inventory[i].type == ItemType::NONE) {
                                    _data->inventory[i].type = ItemType::SCROLL_UPGRADE;
                                    _data->inventory[i].count = 1;
                                    _data->inventory[i].level = 0;
                                    added = true; break;
                                }
                            }
                        }
                        if (added) snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boon: Upg Scroll!");
                        else snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boon Wasted(Pack Full)");
                    }
                    TinyRogueCombat::recalcStats(_data);
                    _data->hudMessageTimer = 60;
                    _data->map[_activeAltarY][_activeAltarX] = TileType::FLOOR;
                    TinyRogueCombat::advanceTurn(_data); // FIX: Advance turn counter to prevent slow monster exploit
                    TinyRogueCombat::processMonsterTurns(_data, ctx, sm, _camera, _particles); // Consumes a turn
                }
            }
        }
        return;
    }

    if (_data->inventoryTurnUsed) {
        _data->inventoryTurnUsed = false;
        TinyRogueCombat::advanceTurn(_data); // FIX: Advance turn counter to prevent slow monster exploit
        TinyRogueCombat::processMonsterTurns(_data, ctx, sm, _camera, _particles);
    }

    if (ctx.justPressed(Btn::MENU1)) {
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
            int px = _data->transforms.data[_data->playerID].x, py = _data->transforms.data[_data->playerID].y;
            int steps = max(abs(aimX - px), abs(aimY - py));
            
            if (steps > 0) {
                for (int i = 1; i <= steps; i++) {
                    int lx = px + (aimX - px) * i / steps;
                    int ly = py + (aimY - py) * i / steps;

                    // Diagonal LoS check: Ensure we aren't cutting through a solid corner
                    int prevX = px + (aimX - px) * (i - 1) / steps;
                    int prevY = py + (aimY - py) * (i - 1) / steps;
                    if (lx != prevX && ly != prevY) {
                        if (_data->map[prevY][lx] == TileType::WALL && _data->map[ly][prevX] == TileType::WALL) {
                             hitWall = true; break;
                        }
                    }

                    if (lx < 0 || lx >= RogueSharedData::MAP_W || ly < 0 || ly >= RogueSharedData::MAP_H ||
                        _data->map[ly][lx] == TileType::WALL || _data->map[ly][lx] == TileType::LOCKED_DOOR) {
                        hitWall = true; 
                        break;
                    }
                }
            }

            if (hitWall) {
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Path Blocked!");
                _data->hudMessageTimer = 40;
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

                EntityID m = TinyRogueCombat::getMonsterAt(_data, aimX, aimY);
                if (m != INVALID_ENTITY) {
                    int dartDamage = _data->combats.data[_data->playerID].baseAttack * 2;
                    if (_data->equippedAccessory.type == ItemType::RING_BERSERKER && _data->healths.data[_data->playerID].hp <= (_data->healths.data[_data->playerID].maxHp * 3) / 10) {
                        dartDamage += 3;
                    }
                    if (dartDamage < 3) dartDamage = 3;
                    _data->healths.data[m].hp -= dartDamage;
                    _data->monsters.data[m].alert = true;
                    TinyRogueCombat::spawnHitEffect(_particles, aimX, aimY);
                    ctx.beep(1200, 30);
                    
                    if (_data->healths.data[m].hp <= 0) {
                        _data->registry.destroy(m);
                        if (_data->monsters.data[m].type == MonsterType::BOSS) {
                            _data->players.data[_data->playerID].xp += 50 + _data->currentDepth * 5;
                            _data->gold += 50 + random(50);
                            
                            _data->map[aimY][aimX] = TileType::STAIRS_DOWN;
                            if (_data->map[aimY + 1][aimX] != TileType::WALL) {
                                _data->map[aimY + 1][aimX] = TileType::CHEST;
                            } else if (_data->map[aimY - 1][aimX] != TileType::WALL) {
                                _data->map[aimY - 1][aimX] = TileType::CHEST;
                            }
                            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boss Defeated!");
                            _data->hudMessageTimer = 80;
                            ctx.beep(1500, 200);
                        } else {
                            _data->players.data[_data->playerID].xp += 10 + _data->currentDepth * 2;
                            if (random(100) < 15) { // 15% chance for gold drop
                                int goldDrop = random(5, 10) + (_data->currentDepth * 2);
                                if (_data->equippedAccessory.type == ItemType::RING_WEALTH) goldDrop += goldDrop / 2;
                                _data->gold += goldDrop;
                            }
                        }
                        
                        // Bloodlust heal
                        int healAmt = (_data->equippedAccessory.type == ItemType::RING_VAMPIRE) ? 2 : 1;
                        _data->healths.data[_data->playerID].hp += healAmt;
                        if (_data->healths.data[_data->playerID].hp > _data->healths.data[_data->playerID].maxHp) _data->healths.data[_data->playerID].hp = _data->healths.data[_data->playerID].maxHp;
                        
                        bool leveledUp = false;
                        while (_data->players.data[_data->playerID].xp >= _data->players.data[_data->playerID].level * 15) {
                            _data->players.data[_data->playerID].xp -= _data->players.data[_data->playerID].level * 15;
                            _data->players.data[_data->playerID].level++;
                            _data->healths.data[_data->playerID].maxHp += 5;
                            _data->healths.data[_data->playerID].hp = _data->healths.data[_data->playerID].maxHp; 
                            _data->combats.data[_data->playerID].baseAttack += 1;
                            TinyRogueCombat::recalcStats(_data);
                            _camera->shake(10);
                            leveledUp = true;
                        }
                        if (leveledUp) {
                            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "LEVEL UP!");
                            _data->hudMessageTimer = 60;
                            ctx.beep(800, 100); ctx.beep(1200, 150);
                        }
                        if (_data->monsters.data[m].type == MonsterType::BOSS) {
                            _data->map[aimY][aimX] = TileType::STAIRS_DOWN;
                            if (_data->map[aimY + 1][aimX] == TileType::FLOOR) {
                                _data->map[aimY + 1][aimX] = TileType::CHEST;
                            }
                            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boss Defeated!");
                            _data->hudMessageTimer = 80;
                            ctx.beep(1500, 200);
                        }
                    }
                } else {
                    // Missed / Threw at empty floor
                    ctx.sfxPoint(); 
                }
                
                isAiming = false;
                TinyRogueCombat::advanceTurn(_data); // FIX: Advance turn counter to prevent slow monster exploit
                TinyRogueCombat::processMonsterTurns(_data, ctx, sm, _camera, _particles);
            }
        }
        else if (ctx.justPressed(Btn::B) ) {
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
        TurnAction action = TinyRogueCombat::processTurn(_data, ctx, sm, dx, dy, _camera, _particles);
        if (action == TurnAction::COMPLETED) {
            TinyRogueCombat::processMonsterTurns(_data, ctx, sm, _camera, _particles); 
        } 
        
        if (action == TurnAction::OPEN_ALTAR) {
            _altarMenuOpen = true;
            _altarMenuCursor = 0;
            _activeAltarX = _data->transforms.data[_data->playerID].x + dx;
            _activeAltarY = _data->transforms.data[_data->playerID].y + dy;
            ctx.sfxMenuEnter();
        } else if (action == TurnAction::OPEN_MERCHANT) {
            ctx.sfxMenuEnter();
            sm.emit(ctx, Event::CUSTOM_2);
        } else if (action == TurnAction::DESCEND_STAIRS) {
            ctx.beep(400, 100); ctx.beep(300, 150); 
            _descending = true;
            _fadeTimer = 20;
        }
    }

    // Universal Death Check
    if (_data->healths.data[_data->playerID].hp <= 0) {
        ctx.sfxDeath();
        if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
        sm.emit(ctx, Event::GAME_OVER);
        return;
    }

    _updateCamera(); 
    
    // Update Fog of War
    int sightRadius = (_data->currentMutator == LevelMutator::PITCH_BLACK) ? 5 : 20;
    for (int y = max(0, _data->transforms.data[_data->playerID].y - 6); y <= min(RogueSharedData::MAP_H - 1, _data->transforms.data[_data->playerID].y + 6); y++) {
        for (int x = max(0, _data->transforms.data[_data->playerID].x - 6); x <= min(RogueSharedData::MAP_W - 1, _data->transforms.data[_data->playerID].x + 6); x++) {
            int distX = abs(x - _data->transforms.data[_data->playerID].x);
            int distY = abs(y - _data->transforms.data[_data->playerID].y);
            if (distX * distX + distY * distY <= sightRadius) {
                _data->explored[y][x] = true;
            }
        }
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
    if ( ctx.justPressed(Btn::B)) {
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
        uint32_t costHealth = 20;
        uint32_t costPotion = 40;
        uint32_t costDarts  = 30;
        uint32_t costScroll = 150;
        
        auto giveItem = [&](ItemType type, int count) -> bool {
            for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                if(_data->inventory[i].type == type) {
                    if ((int)_data->inventory[i].count + count <= 255) {
                        _data->inventory[i].count += count;
                    } else {
                        _data->inventory[i].count = 255;
                    }
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
            if (_data->healths.data[_data->playerID].hp >= _data->healths.data[_data->playerID].maxHp) {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "ALREADY FULL!");
                _msgTimer = 40;
            } else if (_data->gold >= costHealth) {
                _data->gold -= costHealth;
                _data->healths.data[_data->playerID].hp = gclamp(_data->healths.data[_data->playerID].hp + 5, 0, _data->healths.data[_data->playerID].maxHp);
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
    ctx.drawStrRight(bx + bw - 4, by + 9, gBuf);

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
    
    if (_data->hudMessageTimer > 0) {
        int floatY = lerpi(10, 22, _data->hudMessageTimer, 60);
        bool visible = (_data->hudMessageTimer > 15) || (_data->hudMessageTimer % 2 == 0);
        
        if (visible) {
            int w = ctx.strWidth(_data->hudMessage);
            ctx.setDrawColor(0);
            ctx.drawBox((Console::W - w) / 2 - 2, floatY, w + 4, 9);
            ctx.setDrawColor(1);
            ctx.drawStrCentered(floatY + 7, _data->hudMessage);
        }
    }
    
    char topBuf[32];
    snprintf(topBuf, sizeof(topBuf), "%d/%d L:%d", _data->healths.data[_data->playerID].hp, _data->healths.data[_data->playerID].maxHp, _data->players.data[_data->playerID].level);
    int topW = ctx.strWidth(topBuf);
    
    ctx.setDrawColor(0);
    ctx.drawBox(0, 0, 10 + topW + 2, 10);
    ctx.setDrawColor(1);
    ctx.drawBitmap(1, 1, 1, 8, spr_icon_heart);
    ctx.drawStr(11, 7, topBuf);

    char atkBuf[8], defBuf[8];
    snprintf(atkBuf, sizeof(atkBuf), "%d", _data->combats.data[_data->playerID].attack);
    snprintf(defBuf, sizeof(defBuf), "%d", _data->combats.data[_data->playerID].defense);
    
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

    char depBuf[8], goldBuf[16];
    snprintf(depBuf, sizeof(depBuf), "%d", _data->currentDepth);
    snprintf(goldBuf, sizeof(goldBuf), "%d", _data->gold);
    
    int depW = ctx.strWidth(depBuf);
    int goldW = ctx.strWidth(goldBuf);
    int brTotalW = 8 + 2 + depW + 4 + 8 + 2 + goldW;
    int brStartX = Console::W - brTotalW - 2;
    
    // Shift bottom HUD up to make room for XP bar
    int botY = Console::H - 13;
    
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

    // Draw XP Bar at the bottom edge
    int xpNeeded = _data->players.data[_data->playerID].level * 15;
    int xpWidth = (Console::W * _data->players.data[_data->playerID].xp) / xpNeeded;
    ctx.setDrawColor(0);
    ctx.drawBox(0, Console::H - 3, Console::W, 3);
    ctx.setDrawColor(1);
    ctx.drawFrame(0, Console::H - 3, Console::W, 3);
    if (xpWidth > 0) {
        ctx.drawBox(0, Console::H - 2, xpWidth, 1);
    }

    if (_altarMenuOpen) {
        ctx.setCamera(nullptr);
        ctx.setDrawColor(0);
        // Shift Altar menu up by 2px to prevent overlap with the raised HUD
        ctx.drawBox(18, 14, 92, 36);
        ctx.setDrawColor(1);
        ctx.drawFrame(18, 14, 92, 36);
        
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStr(22, 23, "Blood Altar:");
        ctx.drawStr(22, 31, "-5 HP for a Boon?");
        
        ctx.drawStr(32, 41, "Sacrifice");
        ctx.drawStr(32, 49, "Leave");
        
        ctx.drawStr(24, 41 + (_altarMenuCursor * 8), ">");
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

            int distX = abs(mapX - _data->transforms.data[_data->playerID].x);
            int distY = abs(mapY - _data->transforms.data[_data->playerID].y);
            int sightRadius = (_data->currentMutator == LevelMutator::PITCH_BLACK) ? 5 : 20;
            bool inSight = (distX * distX + distY * distY <= sightRadius); 
            
            if (!_data->explored[mapY][mapX]) continue; 

            int renderX = (mapX * 8);
            int renderY = (mapY * 8); 

            TileType t = _data->map[mapY][mapX];

            if (t == TileType::WALL) {
                if (_data->currentBiome == Biome::SEWERS) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_wall_sewer);
                else if (_data->currentBiome == Biome::DEEP_CAVES) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_wall_cave);
                else ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_wall); // Default/Prison
            } else if (t == TileType::FLOOR || t == TileType::CORRIDOR) {
                if (_data->currentBiome == Biome::SEWERS) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_floor_sewer);
                else if (_data->currentBiome == Biome::DEEP_CAVES) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_floor_cave);
                else ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_floor); // Default/Prison
            } else if (t == TileType::WATER) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_water);
            } else if (t == TileType::RUBBLE) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_rubble);
            } else if (t == TileType::WEB) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_web);
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
            } else if (t == TileType::ALTAR) {
                ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_altar);
            }
            EntityID m = TinyRogueCombat::getMonsterAt(_data, mapX, mapY);
            if (m != INVALID_ENTITY && inSight) {
                ctx.setDrawColor(0);
                ctx.drawBox(renderX, renderY, 8, 8);
                ctx.setDrawColor(1);
                
                if (_data->monsters.data[m].type == MonsterType::RAT) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_rat);
                else if (_data->monsters.data[m].type == MonsterType::GOBLIN) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_goblin);
                else if (_data->monsters.data[m].type == MonsterType::BAT) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_bat);
                else if (_data->monsters.data[m].type == MonsterType::SKELETON) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_skeleton);
                else if (_data->monsters.data[m].type == MonsterType::ORC) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_orc);
                else if (_data->monsters.data[m].type == MonsterType::TROLL) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_troll);
                else if (_data->monsters.data[m].type == MonsterType::BOSS) ctx.drawBitmap(renderX, renderY, 1, 8, spr_rogue_boss);
                
                // Draw sleeping indicator 'z' if unaware of player
                if (!_data->monsters.data[m].alert && (millis() / 500) % 2 == 0) {
                    ctx.setFont(u8g2_font_5x7_tf);
                    ctx.drawStr(renderX + 2, renderY - 1, "z");
                }

                // Draw HP Bar if damaged
                if (_data->healths.data[m].hp < _data->healths.data[m].maxHp) {
                    int hpWidth = max(1, (_data->healths.data[m].hp * 6) / _data->healths.data[m].maxHp);
                    ctx.setDrawColor(0);
                    ctx.drawHLine(renderX + 1, renderY + 7, 6);
                    ctx.setDrawColor(1);
                    ctx.drawHLine(renderX + 1, renderY + 7, hpWidth);
                }
            } 
            else if (mapX == _data->transforms.data[_data->playerID].x && mapY == _data->transforms.data[_data->playerID].y) {
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
        int px = (_data->transforms.data[_data->playerID].x * 8) + 4;
        int py = (_data->transforms.data[_data->playerID].y * 8) + 4;
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
    
    if ( ctx.justPressed(Btn::B) || ctx.justPressed(Btn::A)) {
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
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::UP)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // PlayScene
    }
}

void RogueDeadScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int bx = 20, by = 16, bw = 88, bh = 40;

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
    ctx.setFont(u8g2_font_5x7_tf);
    char stats[32];
    snprintf(stats, sizeof(stats), "D: %d  G: %d", _data->currentDepth, _data->gold);
    int sw = ctx.strWidth(stats);
    ctx.drawStr(bx + (bw - sw) / 2, by + 23, stats);

    if ((_frame / 15) % 2 == 0) {
        tw = ctx.strWidth("A to restart");
        ctx.drawStrCentered(by + 35, "A to restart");
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
    _itemMenuOpen = false;
    _itemMenuCursor = 0;
}

void RogueInventoryScene::update(Console& ctx, SceneManager& sm, float dt) {

    if (_upgrading) {
        if (ctx.justPressed(Btn::UP) || ctx.justPressed(Btn::DOWN)) {
            _upgradeSelect = (_upgradeSelect == 0) ? 1 : 0;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::B) ) {
            _upgrading = false;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::A)) {
            Item* target = (_upgradeSelect == 0) ? &_data->equippedWeapon : &_data->equippedArmor;
            if (target && target->type != ItemType::NONE) {
                if (target->level < 255) {
                    target->level++;
                }
                TinyRogueCombat::recalcStats(_data);
                
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

    if (_itemMenuOpen) {
        if (ctx.justPressed(Btn::UP) || ctx.justPressed(Btn::DOWN)) {
            _itemMenuCursor = (_itemMenuCursor == 0) ? 1 : 0;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::B)  || ctx.justPressed(Btn::LEFT)) {
            _itemMenuOpen = false;
            ctx.sfxMenuNav();
        }
        if (ctx.justPressed(Btn::A)) {
            Item& item = _data->inventory[_cursor];
            if (_itemMenuCursor == 1) { // Discard
                item.count--;
                if (item.count <= 0) item.type = ItemType::NONE;
                _cleanInventory();
                snprintf(_msg, sizeof(_msg), "Discarded!");
                _msgTimer = 40;
                ctx.beep(200, 100);
            } else { // Action (Use / Equip / Merge)
                bool consumed = false;
                
                if (item.type == ItemType::POTION) {
                    _data->healths.data[_data->playerID].hp = gclamp(_data->healths.data[_data->playerID].hp + 15, 0, _data->healths.data[_data->playerID].maxHp);
                    snprintf(_msg, sizeof(_msg), "Healed 15 HP!");
                    _data->inventoryTurnUsed = true;
                    consumed = true;
                } else if (item.type == ItemType::ELIXIR) {
                    _data->healths.data[_data->playerID].maxHp += 5;
                    _data->healths.data[_data->playerID].hp += 5;
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
                        int newLevel = (int)_data->equippedWeapon.level + item.level + 1;
                        _data->equippedWeapon.level = (newLevel > 255) ? 255 : newLevel; // Merge!
                        TinyRogueCombat::recalcStats(_data);
                        snprintf(_msg, sizeof(_msg), "Weapons Merged!");
                        ctx.beep(1200, 40); ctx.beep(1500, 60);
                        consumed = true;
                        _data->inventoryTurnUsed = true;
                    } else {
                        Item temp = _data->equippedWeapon;
                        _data->equippedWeapon = item;
                        _data->inventory[_cursor] = temp;
                        TinyRogueCombat::recalcStats(_data);
                        snprintf(_msg, sizeof(_msg), "Equipped Weapon!");
                        _data->inventoryTurnUsed = true;
                        ctx.sfxPoint();
                        _msgTimer = 60;
                    }
                } else if (item.type == ItemType::LEATHER || item.type == ItemType::CHAINMAIL || item.type == ItemType::PLATE) {
                    if (_data->equippedArmor.type == item.type) {
                        int newLevel = (int)_data->equippedArmor.level + item.level + 1;
                        _data->equippedArmor.level = (newLevel > 255) ? 255 : newLevel; // Merge!
                        TinyRogueCombat::recalcStats(_data);
                        snprintf(_msg, sizeof(_msg), "Armors Merged!");
                        ctx.beep(1200, 40); ctx.beep(1500, 60);
                        consumed = true;
                        _data->inventoryTurnUsed = true;
                    } else {
                        Item temp = _data->equippedArmor;
                        _data->equippedArmor = item;
                        _data->inventory[_cursor] = temp;
                        TinyRogueCombat::recalcStats(_data);
                        snprintf(_msg, sizeof(_msg), "Equipped Armor!");
                        _data->inventoryTurnUsed = true;
                        ctx.sfxPoint();
                        _msgTimer = 60;
                    }
                } else if (item.type >= ItemType::RING_VAMPIRE && item.type <= ItemType::RING_BERSERKER) {
                    Item temp = _data->equippedAccessory;
                    _data->equippedAccessory = item;
                    _data->inventory[_cursor] = temp;
                    TinyRogueCombat::recalcStats(_data);
                    snprintf(_msg, sizeof(_msg), "Equipped Ring!");
                    _data->inventoryTurnUsed = true;
                    ctx.sfxPoint();
                    _msgTimer = 60;
                } else if (item.type == ItemType::THROWING_DART) {
                    sm.pop(ctx);
                    RoguePlayScene* play = (RoguePlayScene*)sm.current(); // Assuming Play is under inventory
                    play->isAiming = true;
                    play->aimX = _data->transforms.data[_data->playerID].x;
                    play->aimY = _data->transforms.data[_data->playerID].y;
                    _itemMenuOpen = false;
                    return;
                }
                
                if (consumed) {
                    item.count--;
                    if (item.count <= 0) item.type = ItemType::NONE;
                    _msgTimer = 60;
                    ctx.sfxPoint();
                }
                _cleanInventory();
            }
            _itemMenuOpen = false;
        }
        return;
    }

    if ( ctx.justPressed(Btn::B)) {
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

    if (ctx.justPressed(Btn::A)) {
        if (_data->inventory[_cursor].type != ItemType::NONE) {
            _itemMenuOpen = true;
            _itemMenuCursor = 0;
            ctx.sfxMenuEnter();
        }
    }
}

void RogueInventoryScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    
    int bx = 2, by = 2, bw = 124, bh = 60;
    
    // Main Window Background & Border
    ctx.setDrawColor(0);
    ctx.drawBox(bx + 2, by + 2, bw, bh); // Drop Shadow
    ctx.drawBox(bx, by, bw, bh);         // Background Fill
    ctx.setDrawColor(1);
    ctx.drawFrame(bx, by, bw, bh);       // Main Border
    
    // Header
    ctx.drawBox(bx, by, bw, 9);          // Header inverted bg
    ctx.setFont(u8g2_font_5x7_tf);
    ctx.setDrawColor(0);                 // Black text on white header
    ctx.drawStr(bx + 4, by + 7, "INVENTORY");
    int exitW = ctx.strWidth("[B]Exit");
    ctx.drawStr(bx + bw - exitW - 2, by + 7, "[B]Exit");
    
    ctx.setDrawColor(1); // Back to white on black

    auto getShortName = [](ItemType type) -> const char* {
        switch (type) {
            case ItemType::CHAINMAIL: return "Chain";
            case ItemType::LEATHER: return "Lthr";
            case ItemType::DAGGER: return "Dagr";
            case ItemType::SWORD: return "Swrd";
            case ItemType::AXE: return "Axe";
            case ItemType::SCROLL_UPGRADE: return "Upg Scrl";
            case ItemType::RING_VAMPIRE: return "Vamp Rg";
            case ItemType::RING_WEALTH: return "Wlth Rg";
            case ItemType::RING_OWL: return "Owl Rg";
            case ItemType::RING_BERSERKER: return "Bersk Rg";
            case ItemType::THROWING_DART: return "Dart";
            default: return TinyRogueCombat::getItemName(type);
        }
    };

    // --- Section 1: Equipped Gear ---
    char wStr[32];
    if (_data->equippedWeapon.type != ItemType::NONE) {
        snprintf(wStr, sizeof(wStr), "%s+%d(+%d)", getShortName(_data->equippedWeapon.type), _data->equippedWeapon.level, TinyRogueCombat::getWeaponAttack(_data->equippedWeapon.type) + _data->equippedWeapon.level);
    } else {
        snprintf(wStr, sizeof(wStr), "None");
    }
    ctx.drawBitmap(bx + 4, by + 11, 1, 8, spr_icon_sword);
    ctx.drawStr(bx + 14, by + 18, wStr);

    char aStr[32];
    if (_data->equippedArmor.type != ItemType::NONE) {
        snprintf(aStr, sizeof(aStr), "%s+%d(+%d)", getShortName(_data->equippedArmor.type), _data->equippedArmor.level, TinyRogueCombat::getArmorDefense(_data->equippedArmor.type) + _data->equippedArmor.level);
    } else {
        snprintf(aStr, sizeof(aStr), "None");
    }
    ctx.drawBitmap(bx + 4, by + 20, 1, 8, spr_icon_shield);
    ctx.drawStr(bx + 14, by + 27, aStr);

    char acStr[32];
    if (_data->equippedAccessory.type != ItemType::NONE) {
        snprintf(acStr, sizeof(acStr), "Ac:%s", getShortName(_data->equippedAccessory.type));
    } else {
        snprintf(acStr, sizeof(acStr), "Ac:None");
    }
    ctx.drawStr(bx + 64, by + 18, acStr);
    
    // Gold & Keys
    char gStr[32];
    snprintf(gStr, sizeof(gStr), "G:%d K:%d", _data->gold, _data->keys);
    ctx.drawStr(bx + 64, by + 27, gStr);
    
    // Separator line
    ctx.drawHLine(bx, by + 30, bw);

    // --- Section 2: Pack Grid ---
    for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
        int col = i % 2;
        int row = i / 2;
        int itemX = bx + 4 + (col * 60); 
        int itemY = by + 38 + (row * 9);
        
        bool isFocused = (i == _cursor && !_itemMenuOpen && !_upgrading);
        
        if (isFocused) {
            ctx.drawBox(itemX - 2, itemY - 7, 58, 9);
            ctx.setDrawColor(0); // Invert text color
        }
        
        Item& item = _data->inventory[i];
        char nameBuf[32];
        if (item.type == ItemType::NONE) {
            snprintf(nameBuf, sizeof(nameBuf), "- Empty -");
        } else {
            const char* shortName = getShortName(item.type);
            int atk = TinyRogueCombat::getWeaponAttack(item.type);
            int def = TinyRogueCombat::getArmorDefense(item.type);
            
            if (atk > 0 || def > 0) snprintf(nameBuf, sizeof(nameBuf), "%s+%d", shortName, item.level);
            else snprintf(nameBuf, sizeof(nameBuf), "%s x%d", shortName, item.count);
        }
        
        // Center text slightly if empty, or align left if item
        if (item.type == ItemType::NONE) {
             int nw = ctx.strWidth(nameBuf);
             ctx.drawStr(itemX + (54 - nw)/2, itemY, nameBuf);
        } else {
             ctx.drawStr(itemX + 2, itemY, nameBuf);
        }
        
        if (isFocused) {
            ctx.setDrawColor(1); // Restore white
        }
    }

    // --- Overlays (Action Menu, Upgrading, Messages) ---
    if (_upgrading) {
        int mx = 24, my = 14, mw = 80, mh = 36;
        ctx.setDrawColor(0);
        ctx.drawBox(mx + 2, my + 2, mw, mh); // shadow
        ctx.drawBox(mx, my, mw, mh);         // bg
        ctx.setDrawColor(1);
        ctx.drawFrame(mx, my, mw, mh);       // frame
        
        int tw = ctx.strWidth("Upgrade Target:");
        ctx.drawStr(mx + (mw - tw)/2, my + 9, "Upgrade Target:");
        
        for (int i = 0; i < 2; i++) {
            int optY = my + 20 + (i * 11);
            if (_upgradeSelect == i) {
                ctx.drawBox(mx + 2, optY - 7, mw - 4, 10);
                ctx.setDrawColor(0);
            }
            const char* optStr = (i == 0) ? "Weapon" : "Armor";
            int optW = ctx.strWidth(optStr);
            ctx.drawStr(mx + (mw - optW)/2, optY + 1, optStr);
            if (_upgradeSelect == i) ctx.setDrawColor(1);
        }
    }
    else if (_itemMenuOpen) {
        int mx = 38, my = 22, mw = 52, mh = 26;
        ctx.setDrawColor(0);
        ctx.drawBox(mx + 2, my + 2, mw, mh); // shadow
        ctx.drawBox(mx, my, mw, mh);         // bg
        ctx.setDrawColor(1);
        ctx.drawFrame(mx, my, mw, mh);       // frame
        
        Item& item = _data->inventory[_cursor];
        const char* actStr = "Use";
        if (item.type >= ItemType::DAGGER && item.type <= ItemType::AXE) {
            actStr = (_data->equippedWeapon.type == item.type) ? "Merge" : "Equip";
        } else if (item.type >= ItemType::LEATHER && item.type <= ItemType::PLATE) {
            actStr = (_data->equippedArmor.type == item.type) ? "Merge" : "Equip";
        } else if (item.type >= ItemType::RING_VAMPIRE && item.type <= ItemType::RING_BERSERKER) {
            actStr = "Equip";
        }
        
        for (int i = 0; i < 2; i++) {
            int optY = my + 10 + (i * 11);
            if (_itemMenuCursor == i) {
                ctx.drawBox(mx + 2, optY - 7, mw - 4, 10);
                ctx.setDrawColor(0);
            }
            const char* str = (i == 0) ? actStr : "Discard";
            int tw = ctx.strWidth(str);
            ctx.drawStr(mx + (mw - tw)/2, optY + 1, str);
            if (_itemMenuCursor == i) ctx.setDrawColor(1);
        }
    } 
    else if (_msgTimer > 0) {
        ctx.setDrawColor(1);
        ctx.drawBox(bx + 4, by + bh - 14, bw - 8, 11); // solid white box
        ctx.setDrawColor(0);                           // black text
        int mw = ctx.strWidth(_msg);
        ctx.drawStr(bx + (bw - mw)/2, by + bh - 6, _msg);
        ctx.setDrawColor(1);
        ctx.drawFrame(bx + 4, by + bh - 14, bw - 8, 11);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// TinyRogueGame - OS Registration Hook
// ═════════════════════════════════════════════════════════════════════════════

void TinyRogueGame::onEnter(Console& ctx) { ctx.setCPUSpeed(80);
    _data.hiScore = ctx.loadHiScore();
    
    bool loaded = false;
    if (ctx.hasSave("gamestate")) {
        size_t bytesRead = ctx.loadBytes("gamestate", &_data, sizeof(RogueSharedData));
        if (bytesRead == sizeof(RogueSharedData) && _data.healths.data[_data.playerID].hp > 0) {
            loaded = true;
        }
    }

    _play.setData(&_data);
    if (loaded) _play.resumeSavedGame();
    _play.setEngine(&_camera, &_particles);
    
    _shop.setData(&_data);
    _dead.setData(&_data);
    _inventory.setData(&_data);

    useDefaultEvents(&_pause, &_dead);
    setSnapshotScene(&_play); // Enable Snapshot Support via OS Quick Settings
    
    _sm.onEvent(Event::CUSTOM_1,  SceneManager::REPLACE, &_play); // Start/Restart Game
    _sm.onEvent(Event::CUSTOM_2,  SceneManager::PUSH, &_shop);    // Enter Shop
    _sm.onEvent(Event::CUSTOM_3,  SceneManager::PUSH, &_inventory); // Enter Inventory

    _sm.replace(&_title, ctx);
}

void TinyRogueGame::onExit(Console& ctx) {
    ctx.saveHiScore(_data.hiScore);
    if (_data.healths.data[_data.playerID].hp > 0) {
        ctx.saveBytes("gamestate", &_data, sizeof(RogueSharedData));
    } else {
        ctx.removeSave("gamestate");
    }
    SceneGame<RogueSharedData>::onExit(ctx);
}
const char* TinyRogueGame::getName()   const { return "Tiny Rogue"; }
const uint8_t* TinyRogueGame::getCoverArt() const { return spr_tinyrogue_cover; }

REGISTER_GAME(TinyRogueGame);
