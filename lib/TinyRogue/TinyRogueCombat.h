#pragma once
#include "TinyRogueGame.h"

namespace TinyRogueCombat {
    void spawnHitEffect(ParticleManager* _particles, int gridX, int gridY);
    void advanceTurn(RogueSharedData* _data);
    void recalcStats(RogueSharedData* _data);
    Monster* getMonsterAt(RogueSharedData* _data, int x, int y);
    void processMonsterTurns(RogueSharedData* _data, Console& ctx, SceneManager& sm, Camera* _camera, ParticleManager* _particles);
    TurnAction processTurn(RogueSharedData* _data, Console& ctx, SceneManager& sm, int dx, int dy, Camera* _camera, ParticleManager* _particles);
    
    int getWeaponAttack(ItemType t);
    int getArmorDefense(ItemType t);
    const char* getItemName(ItemType t);
}
