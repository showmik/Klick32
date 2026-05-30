#include "TinyRogueCombat.h"

namespace TinyRogueCombat {

int getWeaponAttack(ItemType t) {
    if (t == ItemType::DAGGER) return 1;
    if (t == ItemType::SWORD) return 2;
    if (t == ItemType::AXE) return 3;
    return 0;
}

int getArmorDefense(ItemType t) {
    if (t == ItemType::LEATHER) return 1;
    if (t == ItemType::CHAINMAIL) return 2;
    if (t == ItemType::PLATE) return 3;
    return 0;
}

const char* getItemName(ItemType t) {
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
        case ItemType::RING_VAMPIRE: return "Vamp Ring";
        case ItemType::RING_WEALTH: return "Wealth Ring";
        case ItemType::RING_OWL: return "Owl Ring";
        case ItemType::RING_BERSERKER: return "Berserk Ring";
        default: return "-";
    }
}

void recalcStats(RogueSharedData* _data) {
    _data->combats.data[_data->playerID].attack = _data->combats.data[_data->playerID].baseAttack + getWeaponAttack(_data->equippedWeapon.type) + _data->equippedWeapon.level;
    _data->combats.data[_data->playerID].defense = _data->combats.data[_data->playerID].baseDefense + getArmorDefense(_data->equippedArmor.type) + _data->equippedArmor.level;
    
    // Hard Stat Caps & Secondary Stats
    _data->combats.data[_data->playerID].critChance = (_data->equippedWeapon.type == ItemType::DAGGER) ? 25 : 10;
    _data->combats.data[_data->playerID].critChance += _data->equippedWeapon.level * 2;
    if (_data->combats.data[_data->playerID].critChance > 50) _data->combats.data[_data->playerID].critChance = 50;

    _data->combats.data[_data->playerID].dodge = (_data->equippedArmor.type == ItemType::LEATHER) ? 15 : 5;
    if (_data->equippedAccessory.type == ItemType::RING_OWL) _data->combats.data[_data->playerID].dodge += 15;
    if (_data->combats.data[_data->playerID].dodge > 60) _data->combats.data[_data->playerID].dodge = 60;

    // Sword: +1 Passive Defense (Parrying)
    if (_data->equippedWeapon.type == ItemType::SWORD) {
        _data->combats.data[_data->playerID].defense += 1;
    }
}


void spawnHitEffect(ParticleManager* _particles, int gridX, int gridY) {
    int px = (gridX * 8) + 4;
    int py = (gridY * 8) + 4;
    for (int i = 0; i < 4; i++) {
        _particles->spawnPixel(px, py, (random(-20, 21) / 10.0f), (random(-20, 21) / 10.0f), random(5, 12));
    }
}

void advanceTurn(RogueSharedData* _data) {
    _data->turnCount++; 
    if (_data->turnCount % 10 == 0 && _data->healths.data[_data->playerID].hp < _data->healths.data[_data->playerID].maxHp) {
        _data->healths.data[_data->playerID].hp++;
    }
}


EntityID getMonsterAt(RogueSharedData* _data, int x, int y) {
    for (EntityID m = 0; m < RogueSharedData::MAX_ENTITIES; m++) {
        if (_data->registry.isValid(m) && _data->monsters.has[m]) {
            if (_data->transforms.data[m].x == x && _data->transforms.data[m].y == y) return m;
        }
    }
    return INVALID_ENTITY;
}

void processMonsterTurns(RogueSharedData* _data, Console& ctx, SceneManager& sm, Camera* _camera, ParticleManager* _particles) {
    bool playerHit = false;

    for (EntityID e = 0; e < RogueSharedData::MAX_ENTITIES; e++) {
        if (!_data->registry.isValid(e) || !_data->monsters.has[e] || _data->healths.data[_data->playerID].hp <= 0) continue;
        
        CMonster& m = _data->monsters.data[e];
        CTransform& t = _data->transforms.data[e];
        CHealth& h = _data->healths.data[e];
        CCombat& c = _data->combats.data[e];

        if (m.spawnTurn > 0) {
            m.spawnTurn--;
            continue;
        }

        int aggro = 6;
        if (m.type == MonsterType::BAT) aggro = 8;
        else if (m.type == MonsterType::SKELETON) aggro = 5;
        else if (m.type == MonsterType::BOSS) aggro = 15;

        if (_data->map[_data->transforms.data[_data->playerID].y][_data->transforms.data[_data->playerID].x] == TileType::TALL_GRASS) {
            aggro = 2; 
        }

        int dx = _data->transforms.data[_data->playerID].x - t.x;
        int dy = _data->transforms.data[_data->playerID].y - t.y;
        int distSq = dx*dx + dy*dy;

        if (distSq <= aggro*aggro) {
            m.alert = true; 
        } else if (distSq > 4) { // 2 tiles away
            if (_data->map[_data->transforms.data[_data->playerID].y][_data->transforms.data[_data->playerID].x] == TileType::TALL_GRASS) {
                m.alert = false;
            }
        }

        if (!m.alert) continue;

        if (distSq == 1) { // Adjacent
            int dmg = (c.attack * 100) / (100 + _data->combats.data[_data->playerID].defense * 8);
            if (dmg < 1) dmg = 1;

            int currentDodge = (_data->map[_data->transforms.data[_data->playerID].y][_data->transforms.data[_data->playerID].x] == TileType::WATER) ? 0 : _data->combats.data[_data->playerID].dodge;
            if (random(100) < currentDodge) {
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Dodged!");
                _data->hudMessageTimer = 30;
                ctx.beep(400, 20);
            } else {
                _data->healths.data[_data->playerID].hp -= dmg;
                _camera->shake(4);
                spawnHitEffect(_particles, _data->transforms.data[_data->playerID].x, _data->transforms.data[_data->playerID].y);
                ctx.beep(150, 100);
                playerHit = true;
            }
        } else {
            // Move logic
            int stepX = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
            int stepY = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;

            if (m.type == MonsterType::RAT && (abs(dx) > 3 || abs(dy) > 3)) {
                if (random(2) == 0) stepX = random(3) - 1;
                else stepY = random(3) - 1;
            }

            if (abs(dx) > abs(dy)) stepY = 0; else stepX = 0;

            if (stepX == 0 && stepY == 0) continue; // Should be impossible if distSq > 1 but safe

            int nx = t.x + stepX;
            int ny = t.y + stepY;

            auto isValidMove = [&](int mx, int my) {
                if (mx < 0 || mx >= RogueSharedData::MAP_W || my < 0 || my >= RogueSharedData::MAP_H) return false;
                if (mx == _data->transforms.data[_data->playerID].x && my == _data->transforms.data[_data->playerID].y) return false; // Handled by attack
                if (_data->map[my][mx] == TileType::WALL || _data->map[my][mx] == TileType::LOCKED_DOOR) return false;
                if (getMonsterAt(_data, mx, my) != INVALID_ENTITY) return false;
                return true;
            };

            if (!isValidMove(nx, ny)) {
                // Try fallback axis
                int altStepX = (stepX == 0) ? ((dx > 0) ? 1 : (dx < 0) ? -1 : 0) : 0;
                int altStepY = (stepY == 0) ? ((dy > 0) ? 1 : (dy < 0) ? -1 : 0) : 0;
                if (isValidMove(t.x + altStepX, t.y + altStepY)) {
                    nx = t.x + altStepX;
                    ny = t.y + altStepY;
                }
            }

            if (isValidMove(nx, ny)) {
                if (_data->map[ny][nx] == TileType::SPIKE) {
                    h.hp -= 2;
                    spawnHitEffect(_particles, nx, ny);
                    ctx.beep(1200, 20);
                    m.alert = true;
                    if (h.hp <= 0) {
                        _data->registry.destroy(e);
                        continue;
                    }
                } else if (_data->map[ny][nx] == TileType::WEB) {
                    _data->map[ny][nx] = TileType::FLOOR;
                    m.spawnTurn = 2; // Monster is stuck in web for 2 turns
                }

                t.x = nx;
                t.y = ny;
            }

        }
    }
}


TurnAction processTurn(RogueSharedData* _data, Console& ctx, SceneManager& sm, int dx, int dy, Camera* _camera, ParticleManager* _particles) {
    auto finalizeTurn = [&]() {
        advanceTurn(_data);
        return TurnAction::COMPLETED;
    };

    if (_data->players.data[_data->playerID].rootDuration > 0) {
        _data->players.data[_data->playerID].rootDuration--;
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Stuck in Web!");
        _data->hudMessageTimer = 30;
        ctx.beep(150, 50);
        return finalizeTurn();
    }

    if (dx == 0 && dy == 0) {
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Waiting...");
        _data->hudMessageTimer = 20;
        return finalizeTurn();
    }

    int targetX = _data->transforms.data[_data->playerID].x + dx;
    int targetY = _data->transforms.data[_data->playerID].y + dy;

    if (targetX < 0 || targetX >= RogueSharedData::MAP_W || targetY < 0 || targetY >= RogueSharedData::MAP_H) return TurnAction::NONE;

    TileType targetTile = _data->map[targetY][targetX];
    if (targetTile == TileType::WALL) return TurnAction::NONE; 
    if (targetTile == TileType::RUBBLE) return TurnAction::NONE; // Impassable but doesn't block LOS

    EntityID targetMonster = getMonsterAt(_data, targetX, targetY);
    CHealth* tmHealth = nullptr;
    CCombat* tmCombat = nullptr;
    CMonster* tmData = nullptr;
    if (targetMonster != INVALID_ENTITY) {
        tmHealth = &_data->healths.data[targetMonster];
        tmCombat = &_data->combats.data[targetMonster];
        tmData = &_data->monsters.data[targetMonster];
    }
    
    if (targetTile == TileType::KEY) {
        _data->keys++;
        _data->map[targetY][targetX] = TileType::FLOOR;
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Found a Key!");
        _data->hudMessageTimer = 60;
        ctx.beep(1200, 50);
    }

    if (targetMonster != INVALID_ENTITY) {
        int rawDmg = _data->combats.data[_data->playerID].attack;
        if (_data->equippedAccessory.type == ItemType::RING_BERSERKER && _data->healths.data[_data->playerID].hp <= (_data->healths.data[_data->playerID].maxHp * 3) / 10) {
            rawDmg += 3;
        }
        
        // Combat Formula: Diminishing returns from monster defense
        int dmg = (rawDmg * 100) / (100 + tmCombat->defense * 8);
        if (dmg < 1) dmg = 1;

        bool crit = false;

        if (!tmData->alert) {
            crit = true; // Guaranteed Sneak Attack!
            tmData->alert = true;
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Sneak Attack!");
            _data->hudMessageTimer = 40;
        } else {
            crit = (random(100) < _data->combats.data[_data->playerID].critChance);
        }

        if (crit) dmg *= 2;

        tmHealth->hp -= dmg;
        _camera->shake(crit ? 6 : 3); 
        spawnHitEffect(_particles, targetX, targetY);
        ctx.beep(crit ? 1500 : 1000, 20); 

        // Axe: Cleave Attack (50% damage to adjacent enemies)
        if (_data->equippedWeapon.type == ItemType::AXE) {
            int cleaveDmg = dmg / 2;
            if (cleaveDmg < 1) cleaveDmg = 1;
            bool cleaved = false;
            for (EntityID e = 0; e < RogueSharedData::MAX_ENTITIES; e++) {
                if (_data->registry.isValid(e) && _data->monsters.has[e] && e != targetMonster) {
                    CTransform& mT = _data->transforms.data[e];
                    CHealth& mH = _data->healths.data[e];
                    CMonster& mM = _data->monsters.data[e];
                    // Check if adjacent to the primary target
                    if (abs(mT.x - targetX) <= 1 && abs(mT.y - targetY) <= 1) {
                        mH.hp -= cleaveDmg;
                        spawnHitEffect(_particles, mT.x, mT.y);
                        mM.alert = true;
                        cleaved = true;
                    }
                }
            }
            if (cleaved) ctx.beep(1200, 30);
        }

        // Check for deaths (Primary target + Cleaved targets)
        int xpGained = 0;
        bool bossDefeated = false;
        int bossX = 0, bossY = 0;

        for (EntityID e = 0; e < RogueSharedData::MAX_ENTITIES; e++) {
            if (_data->registry.isValid(e) && _data->monsters.has[e] && _data->healths.data[e].hp <= 0) {
                CMonster& m = _data->monsters.data[e];
                _data->registry.destroy(e);
                if (m.type == MonsterType::BOSS) {
                    xpGained += 50 + _data->currentDepth * 5;
                    _data->gold += 50 + random(50);
                } else {
                    xpGained += 10 + _data->currentDepth * 2;
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

                if (m.type == MonsterType::BOSS) {
                    bossDefeated = true;
                    bossX = _data->transforms.data[e].x;
                    bossY = _data->transforms.data[e].y;
                }
            }
        }

        if (xpGained > 0) {
            _data->players.data[_data->playerID].xp += xpGained;

            bool leveledUp = false;
            while (_data->players.data[_data->playerID].xp >= _data->players.data[_data->playerID].level * 15) {
                _data->players.data[_data->playerID].xp -= _data->players.data[_data->playerID].level * 15;
                _data->players.data[_data->playerID].level++;
                _data->healths.data[_data->playerID].maxHp += 5;
                _data->healths.data[_data->playerID].hp = _data->healths.data[_data->playerID].maxHp; 
                _data->combats.data[_data->playerID].baseAttack += 1;
                recalcStats(_data);
                leveledUp = true;
            }
            if (leveledUp) {
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "LEVEL UP!");
                _data->hudMessageTimer = 60;
                ctx.beep(800, 100); ctx.beep(1200, 150);
            }
            if (bossDefeated) {
                _data->map[bossY][bossX] = TileType::STAIRS_DOWN;
                if (_data->map[bossY + 1][bossX] != TileType::WALL) {
                    _data->map[bossY + 1][bossX] = TileType::CHEST;
                } else if (_data->map[bossY - 1][bossX] != TileType::WALL) {
                    _data->map[bossY - 1][bossX] = TileType::CHEST;
                }
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boss Defeated!");
                _data->hudMessageTimer = 80;
                ctx.beep(1500, 200);
            }
        }
        return finalizeTurn();
    }    else if (targetTile == TileType::LOCKED_DOOR) {
        if (_data->keys > 0) {
            _data->keys--;
            _data->map[targetY][targetX] = TileType::CORRIDOR; // Remove door
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Door Unlocked!");
            _data->hudMessageTimer = 60;
            ctx.beep(1000, 100);
            return finalizeTurn(); // Takes a turn to unlock
        } else {
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Locked!");
            _data->hudMessageTimer = 40;
            ctx.beep(150, 100);
            return TurnAction::NONE; // Free action if you bump it without a key
        }
    }
    else if (targetTile == TileType::CHEST) {
        // Prevent mimics on Boss floors (depths 5, 10, 15...)
        if (_data->currentDepth % 5 != 0 && random(100) < 15) {
            _data->map[targetY][targetX] = TileType::FLOOR; 
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "It's a MIMIC!");
            _data->hudMessageTimer = 60;
            ctx.beep(200, 150);
            _camera->shake(8);
            
            EntityID spawnM = _data->registry.create();
            if (spawnM != INVALID_ENTITY) {
                float depthF = (float)_data->currentDepth;
                int maxHp = 15 + (int)(depthF * 4.0f);
                _data->transforms.add(spawnM, {targetX, targetY});
                _data->healths.add(spawnM, {maxHp, maxHp});
                _data->combats.add(spawnM, {3 + (int)(depthF * 1.0f), 1 + (int)(depthF * 0.5f), 3 + (int)(depthF * 1.0f), 1 + (int)(depthF * 0.5f), 0, 10});
                _data->monsters.add(spawnM, {MonsterType::GOBLIN, true, 0});
            }
            return finalizeTurn(); 
        }

        int roll = random(100);
        ItemType itemToGive = ItemType::NONE;
        
        if (roll < 35) { itemToGive = ItemType::NONE; } // 35% chance for Gold
        else if (roll < 50) { itemToGive = ItemType::POTION; }
        else if (roll < 55) { itemToGive = ItemType::ELIXIR; }
        else if (roll < 65) { itemToGive = ItemType::SCROLL_UPGRADE; }
        else if (roll < 75) { 
            if (_data->currentDepth < 3) itemToGive = (random(2)==0) ? ItemType::DAGGER : ItemType::SWORD;
            else if (_data->currentDepth < 6) itemToGive = (random(2)==0) ? ItemType::SWORD : ItemType::AXE;
            else itemToGive = ItemType::AXE;
        } else if (roll < 85) {
            if (_data->currentDepth < 3) itemToGive = (random(2)==0) ? ItemType::LEATHER : ItemType::CHAINMAIL;
            else if (_data->currentDepth < 6) itemToGive = (random(2)==0) ? ItemType::CHAINMAIL : ItemType::PLATE;
            else itemToGive = ItemType::PLATE;
        } else {
            int r = random(4);
            if (r == 0) itemToGive = ItemType::RING_VAMPIRE;
            else if (r == 1) itemToGive = ItemType::RING_WEALTH;
            else if (r == 2) itemToGive = ItemType::RING_OWL;
            else itemToGive = ItemType::RING_BERSERKER;
        }
        
        if (itemToGive != ItemType::NONE) {
            const char* itemName = getItemName(itemToGive);
            bool added = false;
            
            if (itemToGive == ItemType::POTION || itemToGive == ItemType::ELIXIR || itemToGive == ItemType::SCROLL_UPGRADE) {
                for(int i = 0; i < RogueSharedData::MAX_INVENTORY; i++) {
                    if(_data->inventory[i].type == itemToGive) {
                        if (_data->inventory[i].count < 255) {
                            _data->inventory[i].count++;
                        }
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
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Pack Full!");
                _data->hudMessageTimer = 60;
                ctx.beep(150, 100);
                return TurnAction::NONE; // Do not consume chest
            }
            _data->map[targetY][targetX] = TileType::FLOOR; 
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Got %s!", itemName);
            _data->hudMessageTimer = 60;
            ctx.beep(800, 40); ctx.beep(1200, 60);
        } else {
            _data->map[targetY][targetX] = TileType::FLOOR; 
            int amount = 10 + (15 * _data->currentDepth);
            if (_data->currentMutator == LevelMutator::TREASURE_TROVE) amount *= 2;
            if (_data->equippedAccessory.type == ItemType::RING_WEALTH) amount += amount / 2; // +50% Gold
            _data->gold += amount;
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Found %d Gold", amount);
            _data->hudMessageTimer = 60;
            ctx.beep(1200, 20); ctx.beep(1500, 40);
        }
        return finalizeTurn();
    }
    else if (targetTile == TileType::MERCHANT) {
        _data->map[targetY][targetX] = TileType::FLOOR;
        return TurnAction::OPEN_MERCHANT; 
    }
    else if (targetTile == TileType::ALTAR) {
        return TurnAction::OPEN_ALTAR; 
    }
    else {
        _data->transforms.data[_data->playerID].x = targetX;
        _data->transforms.data[_data->playerID].y = targetY;

        if (targetTile == TileType::WEB) {
            _data->map[targetY][targetX] = TileType::FLOOR;
            _data->players.data[_data->playerID].rootDuration = 1;
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Trapped!");
            _data->hudMessageTimer = 40;
            ctx.beep(200, 100);
        }

        if (targetTile == TileType::TALL_GRASS) {
            _data->map[targetY][targetX] = TileType::FLOOR;
            if (random(100) < 20 && _data->healths.data[_data->playerID].hp < _data->healths.data[_data->playerID].maxHp) {
                _data->healths.data[_data->playerID].hp += 2;
                if (_data->healths.data[_data->playerID].hp > _data->healths.data[_data->playerID].maxHp) _data->healths.data[_data->playerID].hp = _data->healths.data[_data->playerID].maxHp;
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Dewdrop: +2 HP");
                _data->hudMessageTimer = 40;
                ctx.beep(1000, 30);
            }
        }

        if (targetTile == TileType::SPIKE) {
            if (_data->equippedAccessory.type == ItemType::RING_OWL) {
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Owl Ring: Float!");
                _data->hudMessageTimer = 40;
            } else {
                int spikeDmg = 2 + (_data->currentDepth / 3);
                if (_data->currentMutator == LevelMutator::TREASURE_TROVE) spikeDmg *= 2;
                _data->healths.data[_data->playerID].hp -= spikeDmg;
                _camera->shake(4);
                ctx.beep(100, 50);
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Stepped on Spikes!");
                _data->hudMessageTimer = 60;
                
                if (_data->healths.data[_data->playerID].hp <= 0) {
                    ctx.sfxDeath();
                    return TurnAction::GAME_OVER;
                }
            }
        }

        if (targetTile == TileType::STAIRS_DOWN) {
            return TurnAction::DESCEND_STAIRS;
        }
        return finalizeTurn();
    }
}

}
