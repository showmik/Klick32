void spawnMonsters(RogueSharedData* _data) {
    for (EntityID e = 0; e < RogueSharedData::MAX_ENTITIES; e++) { 
        if (e != _data->playerID) _data->registry.destroy(e); 
    }

    float depth = (float)_data->currentDepth;

    if (_data->currentDepth % 5 == 0) {
        // Boss Room Setup
        EntityID boss = _data->registry.create();
        _data->monsters.add(boss, {MonsterType::BOSS, true, 0});
        _data->transforms.add(boss, {16, 10});
        int maxHp = 40 + (int)(depth * 8.0f);
        _data->healths.add(boss, {maxHp, maxHp});
        _data->combats.add(boss, {4 + (int)(depth * 1.5f), 2 + (int)(depth * 0.8f), 4 + (int)(depth * 1.5f), 2 + (int)(depth * 0.8f), 0, 10});
        return;
    }

    int targetMonsters = (_data->currentDepth * 2) + 6;
    if (_data->currentMutator == LevelMutator::INFESTED) targetMonsters *= 2;
    
    // limit max entities 
    if (targetMonsters > RogueSharedData::MAX_ENTITIES - 5) {
        targetMonsters = RogueSharedData::MAX_ENTITIES - 5;
    }

    for (int i = 0; i < targetMonsters; i++) {
        int mx, my;
        int attempts = 0;
        do {
            mx = random(1, RogueSharedData::MAP_W - 1);
            my = random(1, RogueSharedData::MAP_H - 1);
            attempts++;
        } while ((_data->map[my][mx] == TileType::WALL || _data->map[my][mx] == TileType::WATER ||
                 (abs(mx - _data->transforms.data[_data->playerID].x) < 5 && abs(my - _data->transforms.data[_data->playerID].y) < 5)) && attempts < 100);

        if (attempts >= 100) continue; 

        EntityID e = _data->registry.create();
        if (e == INVALID_ENTITY) break;

        MonsterType t = MonsterType::RAT;
        int maxHp = 0, atk = 0, def = 0;

        int r = random(100);
        if (_data->currentBiome == Biome::SEWERS) {
            if (r < 50) { t = MonsterType::RAT; maxHp = 6 + depth; atk = 1 + (depth*0.5); def = 0; }
            else if (r < 90) { t = MonsterType::BAT; maxHp = 4 + depth; atk = 2 + (depth*0.5); def = 0; }
            else { t = MonsterType::GOBLIN; maxHp = 8 + depth*2; atk = 2 + depth; def = 1; }
        }
        else if (_data->currentBiome == Biome::PRISON) {
            if (r < 40) { t = MonsterType::SKELETON; maxHp = 10 + depth*2; atk = 3 + depth; def = 1; }
            else if (r < 70) { t = MonsterType::GOBLIN; maxHp = 8 + depth*2; atk = 2 + depth; def = 1; }
            else if (r < 90) { t = MonsterType::ORC; maxHp = 15 + depth*3; atk = 4 + depth; def = 2; }
            else { t = MonsterType::RAT; maxHp = 8 + depth; atk = 2 + depth; def = 0; }
        }
        else { // DEEP_CAVES
            if (r < 40) { t = MonsterType::ORC; maxHp = 18 + depth*3; atk = 5 + depth; def = 2; }
            else if (r < 70) { t = MonsterType::SKELETON; maxHp = 12 + depth*2; atk = 4 + depth; def = 1; }
            else if (r < 95) { t = MonsterType::TROLL; maxHp = 30 + depth*4; atk = 6 + depth; def = 3; }
            else { t = MonsterType::BAT; maxHp = 6 + depth; atk = 3 + depth; def = 0; }
        }
        
        _data->monsters.add(e, {t, false, 0});
        _data->transforms.add(e, {mx, my});
        _data->healths.add(e, {maxHp, maxHp});
        _data->combats.add(e, {atk, def, atk, def, 0, 10});
    }
}
