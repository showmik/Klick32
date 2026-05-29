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
    _data->player.attack = _data->player.baseAttack + getWeaponAttack(_data->equippedWeapon.type) + _data->equippedWeapon.level;
    _data->player.defense = _data->player.baseDefense + getArmorDefense(_data->equippedArmor.type) + _data->equippedArmor.level;
    
    // Hard Stat Caps & Secondary Stats
    _data->player.critChance = (_data->equippedWeapon.type == ItemType::DAGGER) ? 25 : 10;
    _data->player.critChance += _data->equippedWeapon.level * 2;
    if (_data->player.critChance > 50) _data->player.critChance = 50;

    _data->player.dodge = (_data->equippedArmor.type == ItemType::LEATHER) ? 15 : 5;
    if (_data->equippedAccessory.type == ItemType::RING_OWL) _data->player.dodge += 15;
    if (_data->player.dodge > 60) _data->player.dodge = 60;

    // Sword: +1 Passive Defense (Parrying)
    if (_data->equippedWeapon.type == ItemType::SWORD) {
        _data->player.defense += 1;
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
    if (_data->turnCount % 10 == 0 && _data->player.hp < _data->player.maxHp) {
        _data->player.hp++;
    }
}


Monster* getMonsterAt(RogueSharedData* _data, int x, int y) {
    for (auto& m : _data->monsters) {
        if (m.active && m.x == x && m.y == y) return &m;
    }
    return nullptr;
}

void processMonsterTurns(RogueSharedData* _data, Console& ctx, SceneManager& sm, Camera* _camera, ParticleManager* _particles) {
    bool playerHit = false;

    for (auto& m : _data->monsters) {
        if (!m.active || _data->player.hp <= 0) continue;

        if (m.rootDuration > 0) {
            m.rootDuration--;
            continue;
        }

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
                    int currentDodge = (_data->map[_data->player.y][_data->player.x] == TileType::WATER) ? 0 : _data->player.dodge;
                    if (random(100) < currentDodge) {
                        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Dodged!");
                        _data->hudMessageTimer = 30;
                    } else {
                        int rawDmg = m.attack;
                        
                        // Infested Flanking Bonus: +1 damage for each additional adjacent monster
                        if (_data->currentMutator == LevelMutator::INFESTED) {
                            for (auto& otherM : _data->monsters) {
                                if (&otherM != &m && otherM.active && abs(otherM.x - _data->player.x) <= 1 && abs(otherM.y - _data->player.y) <= 1) {
                                    rawDmg += 1;
                                }
                            }
                        }

                        int damage = (rawDmg * 100) / (100 + _data->player.defense * 8);
                        if (damage < 1) damage = 1; 

                        _data->player.hp -= damage;
                        playerHit = true;
                    }
                    break; 
                }
                else if (_data->map[ny][nx] != TileType::WALL && !getMonsterAt(_data, nx, ny)) {
                    if (_data->map[ny][nx] == TileType::RUBBLE) continue;
                    
                    if (_data->map[ny][nx] == TileType::WEB) {
                        _data->map[ny][nx] = TileType::FLOOR;
                        m.rootDuration = 1;
                    }

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
                    int targetX = m.x + (random(3) - 1);
                    int targetY = m.y + (random(3) - 1);
                    
                    // Validate position BEFORE activating
                    if (targetX >= 0 && targetX < RogueSharedData::MAP_W && 
                        targetY >= 0 && targetY < RogueSharedData::MAP_H &&
                        _data->map[targetY][targetX] == TileType::FLOOR && 
                        (targetX != _data->player.x || targetY != _data->player.y) &&
                        !getMonsterAt(_data, targetX, targetY)) {
                        
                        newM.active = true;
                        newM.type = (random(2) == 0) ? MonsterType::SKELETON : MonsterType::GOBLIN;
                        newM.x = targetX;
                        newM.y = targetY;
                        
                        float depthF = (float)_data->currentDepth;
                        newM.maxHp = 8 + (int)(depthF * 2.5f);
                        newM.hp = newM.maxHp;
                        newM.attack = 2 + (int)(depthF * 0.8f);
                        newM.defense = (int)(depthF * 0.4f);
                        newM.alert = true;
                        
                        _camera->shake(5);
                        ctx.beep(200, 100);
                        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Boss Summons!");
                        _data->hudMessageTimer = 60;
                    } else {
                        newM.active = false; // Cancel if spot is invalid
                    }
                    break;
                }
            }
        }
    }

    if (playerHit) {
        ctx.beep(150, 40); 
        _camera->shake(6);
        
        if (_data->player.hp <= 0) {
            if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
            ctx.sfxDeath();
            sm.emit(ctx, Event::GAME_OVER);
        }
    } else {
        ctx.beep(400, 10); 
    }

    // Trample Tall Grass AFTER monsters take their turn so stealth checks work
    if (_data->map[_data->player.y][_data->player.x] == TileType::TALL_GRASS) {
        _data->map[_data->player.y][_data->player.x] = TileType::FLOOR;
    }
}

TurnAction processTurn(RogueSharedData* _data, Console& ctx, SceneManager& sm, int dx, int dy, Camera* _camera, ParticleManager* _particles) {
    auto finalizeTurn = [&]() {
        advanceTurn(_data);
        return TurnAction::COMPLETED;
    };

    if (_data->player.rootDuration > 0) {
        _data->player.rootDuration--;
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

    int targetX = _data->player.x + dx;
    int targetY = _data->player.y + dy;

    if (targetX < 0 || targetX >= RogueSharedData::MAP_W || targetY < 0 || targetY >= RogueSharedData::MAP_H) return TurnAction::NONE;

    TileType targetTile = _data->map[targetY][targetX];
    if (targetTile == TileType::WALL) return TurnAction::NONE; 
    if (targetTile == TileType::RUBBLE) return TurnAction::NONE; // Impassable but doesn't block LOS

    Monster* targetMonster = getMonsterAt(_data, targetX, targetY);
    
    if (targetTile == TileType::KEY) {
        _data->keys++;
        _data->map[targetY][targetX] = TileType::FLOOR;
        snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Found a Key!");
        _data->hudMessageTimer = 60;
        ctx.beep(1200, 50);
    }

    if (targetMonster) {
        int rawDmg = _data->player.attack;
        if (_data->equippedAccessory.type == ItemType::RING_BERSERKER && _data->player.hp <= (_data->player.maxHp * 3) / 10) {
            rawDmg += 3;
        }
        
        // Combat Formula: Diminishing returns from monster defense
        int dmg = (rawDmg * 100) / (100 + targetMonster->defense * 8);
        if (dmg < 1) dmg = 1;

        bool crit = false;

        if (!targetMonster->alert) {
            crit = true; // Guaranteed Sneak Attack!
            targetMonster->alert = true;
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Sneak Attack!");
            _data->hudMessageTimer = 40;
        } else {
            crit = (random(100) < _data->player.critChance);
        }

        if (crit) dmg *= 2;

        targetMonster->hp -= dmg;
        _camera->shake(crit ? 6 : 3); 
        spawnHitEffect(_particles, targetX, targetY);
        ctx.beep(crit ? 1500 : 1000, 20); 

        // Axe: Cleave Attack (50% damage to adjacent enemies)
        if (_data->equippedWeapon.type == ItemType::AXE) {
            int cleaveDmg = dmg / 2;
            if (cleaveDmg < 1) cleaveDmg = 1;
            bool cleaved = false;
            for (auto& m : _data->monsters) {
                if (m.active && &m != targetMonster) {
                    // Check if adjacent to the primary target
                    if (abs(m.x - targetX) <= 1 && abs(m.y - targetY) <= 1) {
                        m.hp -= cleaveDmg;
                        spawnHitEffect(_particles, m.x, m.y);
                        m.alert = true;
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

        for (auto& m : _data->monsters) {
            if (m.active && m.hp <= 0) {
                m.active = false;
                if (m.type == MonsterType::BOSS) {
                    xpGained += 50 + _data->currentDepth * 5;
                    _data->gold += 50 + random(50);
                } else {
                    xpGained += 10 + _data->currentDepth * 2;
                    if (random(100) < 40) {
                        int goldDrop = random(2, 6) + _data->currentDepth;
                        if (_data->equippedAccessory.type == ItemType::RING_WEALTH) goldDrop += goldDrop / 2;
                        _data->gold += goldDrop;
                    }
                }

                // Bloodlust heal
                int healAmt = (_data->equippedAccessory.type == ItemType::RING_VAMPIRE) ? 2 : 1;
                _data->player.hp += healAmt;
                if (_data->player.hp > _data->player.maxHp) _data->player.hp = _data->player.maxHp;

                if (m.type == MonsterType::BOSS) {
                    bossDefeated = true;
                    bossX = m.x;
                    bossY = m.y;
                }
            }
        }

        if (xpGained > 0) {
            _data->player.xp += xpGained;

            bool leveledUp = false;
            while (_data->player.xp >= _data->player.level * 15) {
                _data->player.xp -= _data->player.level * 15;
                _data->player.level++;
                _data->player.maxHp += 5;
                _data->player.hp = _data->player.maxHp; 
                _data->player.baseAttack += 1;
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
                if (_data->map[bossY + 1][bossX] == TileType::FLOOR) {
                    _data->map[bossY + 1][bossX] = TileType::CHEST;
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
            
            Monster* spawnM = nullptr;
            for (auto& m : _data->monsters) {
                if (!m.active) { spawnM = &m; break; }
            }
            if (!spawnM) spawnM = &_data->monsters[RogueSharedData::MAX_MONSTERS - 1]; // Fallback to overwrite last
            
            spawnM->x = targetX; spawnM->y = targetY;
            spawnM->active = true;
            float depthF = (float)_data->currentDepth;
            spawnM->maxHp = 15 + (int)(depthF * 4.0f);
            spawnM->hp = spawnM->maxHp;
            spawnM->attack = 3 + (int)(depthF * 1.0f);
            spawnM->defense = 1 + (int)(depthF * 0.5f);
            spawnM->type = MonsterType::GOBLIN; 
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
        return TurnAction::OPEN_MERCHANT; 
    }
    else if (targetTile == TileType::ALTAR) {
        return TurnAction::OPEN_ALTAR; 
    }
    else {
        _data->player.x = targetX;
        _data->player.y = targetY;

        if (targetTile == TileType::WEB) {
            _data->map[targetY][targetX] = TileType::FLOOR;
            _data->player.rootDuration = 1;
            snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Trapped!");
            _data->hudMessageTimer = 40;
            ctx.beep(200, 100);
        }

        if (targetTile == TileType::TALL_GRASS) {
            if (random(100) < 20 && _data->player.hp < _data->player.maxHp) {
                _data->player.hp += 2;
                if (_data->player.hp > _data->player.maxHp) _data->player.hp = _data->player.maxHp;
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
                _data->player.hp -= spikeDmg;
                _camera->shake(4);
                ctx.beep(100, 50);
                snprintf(_data->hudMessage, sizeof(_data->hudMessage), "Stepped on Spikes!");
                _data->hudMessageTimer = 60;
                
                if (_data->player.hp <= 0) {
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
