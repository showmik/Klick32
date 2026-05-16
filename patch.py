import sys
with open(r'H:\dev\Klick32\lib\TinyRogue\TinyRogueGame.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Fix merchant in _generateBSPMap
old_merchant_bsp = """    // Spawn exactly 1 Merchant on an open floor tile
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

    for (int i = numNodes - 1; i >= 0; i--) {"""
new_merchant_bsp = """    for (int i = numNodes - 1; i >= 0; i--) {"""
content = content.replace(old_merchant_bsp, new_merchant_bsp)

old_bsp_end = """        for (int i = 1; i < leafCount - 1; i++) {
            if (random(100) < 30) {
                Rect r = nodes[leafIndices[i]].room;
                _data->map[r.y + 1][r.x + 1] = TileType::CHEST;
            }
        }
    }
}"""
new_bsp_end = """        for (int i = 1; i < leafCount - 1; i++) {
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
}"""
content = content.replace(old_bsp_end, new_bsp_end)

# Fix merchant in _generateCaveMap
old_merchant_cave = """    // Spawn exactly 1 Merchant on an open floor tile
    int mx, my;
    do {
        mx = random(1, RogueSharedData::MAP_W - 1);
        my = random(1, RogueSharedData::MAP_H - 1);
    } while (_data->map[my][mx] != TileType::FLOOR || (mx == _data->player.x && my == _data->player.y));
    _data->map[my][mx] = TileType::MERCHANT;

    // 3. Helper to find open space"""
new_merchant_cave = """    // 3. Helper to find open space"""
content = content.replace(old_merchant_cave, new_merchant_cave)

old_cave_end = """    int numChests = random(1, 4);
    for (int i = 0; i < numChests; i++) {
        Vec2 c = getOpenTile();
        if (c.ix() != _data->player.x || c.iy() != _data->player.y) {
            _data->map[c.iy()][c.ix()] = TileType::CHEST;
        }
    }
}"""
new_cave_end = """    int numChests = random(1, 4);
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
}"""
content = content.replace(old_cave_end, new_cave_end)

# Fix playerHit Death handling in _processMonsterTurns
old_death = """    if (playerHit) {
        ctx.sfxDeath(); 
        _camera->shake(6);
        
        if (_data->player.hp <= 0) {
        if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
        ctx.sfxDeath();
        sm.emit(ctx, Event::GAME_OVER);
    }    } else {"""
new_death = """    if (playerHit) {
        ctx.sfxDeath(); 
        _camera->shake(6);
        
        if (_data->player.hp <= 0) {
            if (_data->gold > _data->hiScore) _data->hiScore = _data->gold;
            ctx.sfxDeath();
            sm.emit(ctx, Event::GAME_OVER);
        }
    } else {"""
content = content.replace(old_death, new_death)

# Stop monster turns if player dies
old_monster_loop = """    for (auto& m : _data->monsters) {
        if (!m.active) continue;"""
new_monster_loop = """    for (auto& m : _data->monsters) {
        if (!m.active || _data->player.hp <= 0) continue;"""
content = content.replace(old_monster_loop, new_monster_loop)

# Fix level up and XP bug in _processTurn
old_xp = """        if (targetMonster->hp <= 0) {
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
        }"""
new_xp = """        if (targetMonster->hp <= 0) {
            targetMonster->active = false;
            _data->player.xp += targetMonster->maxHp;
            
            bool leveledUp = false;
            while (_data->player.xp >= _data->player.level * 10) {
                _data->player.xp -= _data->player.level * 10;
                _data->player.level++;
                _data->player.maxHp += 5;
                _data->player.hp = _data->player.maxHp; 
                _data->player.attack += 1;
                leveledUp = true;
            }
            if (leveledUp) {
                snprintf(_hudMessage, sizeof(_hudMessage), "LEVEL UP!");
                _hudMessageTimer = 60;
                ctx.beep(800, 100); ctx.beep(1200, 150);
            }
        }"""
content = content.replace(old_xp, new_xp)

# Add HP regen in _processTurn
old_turn = """    _data->turnCount++; 

    int targetX = _data->player.x + dx;
    int targetY = _data->player.y + dy;"""
new_turn = """    _data->turnCount++; 
    
    // Passive HP Regeneration (1 HP every 20 turns)
    if (_data->turnCount % 20 == 0 && _data->player.hp < _data->player.maxHp) {
        _data->player.hp++;
    }

    int targetX = _data->player.x + dx;
    int targetY = _data->player.y + dy;"""
content = content.replace(old_turn, new_turn)

# Update Shop Logic
old_shop_update = """void RogueShopScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (_introFrames < 10) _introFrames++;
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }

    if (ctx.justPressed(Btn::UP) || ctx.repeat(Btn::UP)) {
        if (_cursor > 0) { _cursor--; ctx.sfxMenuNav(); }
    }
    if (ctx.justPressed(Btn::DOWN) || ctx.repeat(Btn::DOWN)) {
        if (_cursor < 2) { _cursor++; ctx.sfxMenuNav(); }
    }

    if (_msgTimer > 0) _msgTimer--;

    if (ctx.justPressed(Btn::A)) {
        int costHealth = 20;
        int costMaxHp = 50;
        if (_cursor == 0) { // Buy Health
            if (_data->gold >= costHealth) {
                _data->gold -= costHealth;
                _data->player.hp = gclamp(_data->player.hp + 5, 0, _data->player.maxHp);
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "BOUGHT HP!");
                _msgTimer = 40;
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 1) { // Buy Max HP
            if (_data->gold >= costMaxHp) {
                _data->gold -= costMaxHp;
                _data->player.maxHp += 5;
                _data->player.hp += 5;
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "MAX HP UP!");
                _msgTimer = 40;
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
}"""

new_shop_update = """void RogueShopScene::update(Console& ctx, SceneManager& sm, float dt) {
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
        int costMaxHp = 50;
        int costAtk = 100;
        int costDef = 100;
        
        if (_cursor == 0) { // Buy Health
            if (_data->player.hp >= _data->player.maxHp) {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "ALREADY FULL!");
                _msgTimer = 40;
            } else if (_data->gold >= costHealth) {
                _data->gold -= costHealth;
                _data->player.hp = gclamp(_data->player.hp + 5, 0, _data->player.maxHp);
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "BOUGHT HP!");
                _msgTimer = 40;
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 1) { // Buy Max HP
            if (_data->gold >= costMaxHp) {
                _data->gold -= costMaxHp;
                _data->player.maxHp += 5;
                _data->player.hp += 5;
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "MAX HP UP!");
                _msgTimer = 40;
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 2) { // Buy ATK
            if (_data->gold >= costAtk) {
                _data->gold -= costAtk;
                _data->player.attack += 1;
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "ATTACK UP!");
                _msgTimer = 40;
            } else {
                ctx.beep(150, 100);
                snprintf(_msg, sizeof(_msg), "NOT ENOUGH!");
                _msgTimer = 40;
            }
        } else if (_cursor == 3) { // Buy DEF
            if (_data->gold >= costDef) {
                _data->gold -= costDef;
                _data->player.defense += 1;
                ctx.sfxPoint();
                snprintf(_msg, sizeof(_msg), "DEFENSE UP!");
                _msgTimer = 40;
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
}"""
content = content.replace(old_shop_update, new_shop_update)

old_shop_draw = """void RogueShopScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int yOff = lerpi(Console::H, 0, _introFrames, 10);
    int bx = 10, by = 8 + yOff, bw = 108, bh = 52; 

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
    ctx.drawStr(bx + 4, by + 11, "MERCHANT");

    ctx.setFont(u8g2_font_5x7_tf);
    int escW = ctx.strWidth("[B] Exit");
    ctx.drawStr(bx + bw - escW - 4, by + 10, "[B] Exit"); 

    ctx.setDrawColor(1);
    
    char gBuf[16]; 
    snprintf(gBuf, sizeof(gBuf), "Wallet: %u g", _data->gold);
    ctx.drawStr(bx + 4, by + 24, gBuf);

    ctx.drawStr(bx + 14, by + 34, "Heal HP   (20g)");
    ctx.drawStr(bx + 14, by + 42, "Up HP     (50g)");
    ctx.drawStr(bx + 14, by + 50, "Exit Shop");

    ctx.drawStr(bx + 4, by + 34 + (_cursor * 8), ">");

    if (_msgTimer > 0) {
        ctx.setDrawColor(0);
        ctx.drawBox(bx + 2, by + 36, bw - 4, 14);
        ctx.setDrawColor(1);
        
        int msgW = ctx.strWidth(_msg);
        ctx.drawStr(bx + (bw - msgW) / 2, by + 46, _msg);
    }
}"""

new_shop_draw = """void RogueShopScene::draw(Console& ctx) {
    if (_sm) _sm->drawUnder(ctx);
    int yOff = lerpi(Console::H, 0, _introFrames, 10);
    int bx = 10, by = 8 + yOff, bw = 108, bh = 76; 

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
    ctx.drawStr(bx + 4, by + 11, "MERCHANT");

    ctx.setFont(u8g2_font_5x7_tf);
    int escW = ctx.strWidth("[B] Exit");
    ctx.drawStr(bx + bw - escW - 4, by + 10, "[B] Exit"); 

    ctx.setDrawColor(1);
    
    char gBuf[16]; 
    snprintf(gBuf, sizeof(gBuf), "Wallet: %u g", _data->gold);
    ctx.drawStr(bx + 4, by + 24, gBuf);

    ctx.drawStr(bx + 14, by + 34, "Heal HP   (20g)");
    ctx.drawStr(bx + 14, by + 42, "Up HP     (50g)");
    ctx.drawStr(bx + 14, by + 50, "Up ATK   (100g)");
    ctx.drawStr(bx + 14, by + 58, "Up DEF   (100g)");
    ctx.drawStr(bx + 14, by + 66, "Exit Shop");

    ctx.drawStr(bx + 4, by + 34 + (_cursor * 8), ">");

    if (_msgTimer > 0) {
        ctx.setDrawColor(0);
        ctx.drawBox(bx + 2, by + bh - 16, bw - 4, 14);
        ctx.setDrawColor(1);
        
        int msgW = ctx.strWidth(_msg);
        ctx.drawStr(bx + (bw - msgW) / 2, by + bh - 6, _msg);
    }
}"""
content = content.replace(old_shop_draw, new_shop_draw)

with open(r'H:\dev\Klick32\lib\TinyRogue\TinyRogueGame.cpp', 'w', encoding='utf-8') as f:
    f.write(content)
print("PATCHED")