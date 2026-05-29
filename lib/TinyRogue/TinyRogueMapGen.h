#pragma once
#include "TinyRogueGame.h"

namespace TinyRogueMapGen {
    void generateMap(RogueSharedData* _data);
    void generateBSPMap(RogueSharedData* _data);
    void generateCaveMap(RogueSharedData* _data);
    void generateMazeMap(RogueSharedData* _data);
    void generateBossMap(RogueSharedData* _data);
    void spawnMonsters(RogueSharedData* _data);
}
