#pragma once
#include "SceneGame.h"

struct DoomData {
    uint32_t hiScore = 0;
    uint32_t score = 0;
    int hp = 100;
    int ammo = 50;
    int level = 0;
    int weapon = 0; // 0 = pistol, 1 = shotgun
};

struct DoomSprite {
    float x;
    float y;
    int type; // 1=skull, 2=medkit, 3=imp, 4=key, 5=ammo
    bool active;
    float distance;
    int hp;
    int flash_timer;
    int state;
};

class DoomTitleScene : public Scene {
public:
    void setShared(DoomData* data) { _data = data; }
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
private:
    DoomData* _data = nullptr;
    int frame_counter = 0;
};

class DoomWinScene : public Scene {
public:
    void setShared(DoomData* data) { _data = data; }
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
private:
    DoomData* _data = nullptr;
};

class DoomPlayScene : public Scene {
public:
    void setShared(DoomData* data) { _data = data; }
    void setEngine(Camera* cam, ParticleManager* anim) { _camera = cam; _particles = anim; }
    
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
    
private:
    void loadLevel(Console& ctx);
    
    DoomData* _data = nullptr;
    Camera* _camera = nullptr;
    ParticleManager* _particles = nullptr;
    
    float player_x = 0;
    float player_y = 0;
    float player_dir = 0;
    
    float weapon_bob = 0;
    int fire_timer = 0;
    
    int frame_counter = 0;
    int damage_timer = 0;
    int level_transition_timer = 0;
    bool show_minimap = false;
    bool has_key = false;
    
    uint8_t map[12][12];
    DoomSprite sprites[30];
    int num_sprites = 0;
};

class DoomGame : public SceneGame<DoomData> {
public:
    void reset() override { *this = DoomGame(); }
    void onEnter(Console& ctx) override;
    const char* getName() const override;

private:
    DoomTitleScene _title;
    DoomPlayScene  _play;
    DoomWinScene   _win;
};
