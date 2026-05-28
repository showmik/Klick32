import os
import sys

def create_game(name):
    base_dir = "lib"
    game_dir = os.path.join(base_dir, f"{name}Game")
    
    if os.path.exists(game_dir):
        print(f"Error: Directory {game_dir} already exists.")
        sys.exit(1)
        
    os.makedirs(game_dir)
    
    header_content = f"""#pragma once
#include "SceneGame.h"

struct {name}Data {{
    uint32_t hiScore = 0;
}};

class {name}TitleScene : public Scene {{
public:
    void setShared({name}Data* data) {{ _data = data; }}
    
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
    
private:
    {name}Data* _data = nullptr;
}};

class {name}PlayScene : public Scene {{
public:
    void setShared({name}Data* data) {{ _data = data; }}
    void setEngine(Camera* cam, ParticleManager* anim) {{ _camera = cam; _particles = anim; }}
    
    void onEnter(Console& ctx) override;
    void update(Console& ctx, SceneManager& sm, float dt) override;
    void draw(Console& ctx) override;
    
private:
    {name}Data* _data = nullptr;
    Camera* _camera = nullptr;
    ParticleManager* _particles = nullptr;
}};

class {name}Game : public SceneGame<{name}Data> {{
public:
    void onEnter(Console& ctx) override;
    const char* getName() const override;

private:
    {name}TitleScene _title;
    {name}PlayScene  _play;
}};
"""

    cpp_content = f"""#include "{name}Game.h"
#include "GameRegistry.h"

// ═════════════════════════════════════════════════════════════════════════════
// {name}TitleScene
// ═════════════════════════════════════════════════════════════════════════════

void {name}TitleScene::onEnter(Console& ctx) {{
    // Setup title screen
}}

void {name}TitleScene::update(Console& ctx, SceneManager& sm, float dt) {{
    if (ctx.justPressed(Btn::A)) {{
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Custom event to transition to play
    }}
    if (ctx.justPressed(Btn::B)) {{
        sm.emit(ctx, Event::QUIT); // Exit game
    }}
}}

void {name}TitleScene::draw(Console& ctx) {{
    ctx.setFont(u8g2_font_6x10_tf);
    ctx.drawStrCenteredBoth("{name} Title");
}}

// ═════════════════════════════════════════════════════════════════════════════
// {name}PlayScene
// ═════════════════════════════════════════════════════════════════════════════

void {name}PlayScene::onEnter(Console& ctx) {{
    // Reset level state
}}

void {name}PlayScene::update(Console& ctx, SceneManager& sm, float dt) {{
    if (ctx.justPressed(Btn::B)) {{
        sm.emit(ctx, Event::QUIT);
    }}
}}

void {name}PlayScene::draw(Console& ctx) {{
    ctx.setFont(u8g2_font_6x10_tf);
    ctx.drawStrCenteredBoth("{name} Gameplay");
}}

// ═════════════════════════════════════════════════════════════════════════════
// {name}Game Boilerplate
// ═════════════════════════════════════════════════════════════════════════════

void {name}Game::onEnter(Console& ctx) {{
    // Load shared data
    _data.hiScore = ctx.loadHiScore();

    // Wire dependencies
    _title.setShared(&_data);
    _play.setShared(&_data);
    _play.setEngine(&_camera, &_particles);

    // Register event transitions
    useDefaultEvents(nullptr, nullptr); // Add pause/game over scenes if needed
    _sm.onEvent(Event::CUSTOM_1, SceneManager::REPLACE, &_play);

    // Start at title
    _sm.replace(&_title, ctx);
}}

const char* {name}Game::getName() const {{
    return "{name}";
}}

REGISTER_GAME({name}Game);
"""

    with open(os.path.join(game_dir, f"{name}Game.h"), "w") as f:
        f.write(header_content)
        
    with open(os.path.join(game_dir, f"{name}Game.cpp"), "w") as f:
        f.write(cpp_content)

    print(f"Successfully scaffolded {name}Game in {game_dir}/")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python create_game.py <GameName>")
        print("Example: python create_game.py Asteroids")
        sys.exit(1)
    
    create_game(sys.argv[1])