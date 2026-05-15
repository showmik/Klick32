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
#include "GameBase.h"

class {name}Game : public GameBase {{
public:
    void onEnter(Console& ctx) override;
    void onExit(Console& ctx) override;
    void update(Console& ctx, float dt) override;
    void draw(Console& ctx) override;
    bool isRunning() const override;
    const char* getName() const override;

private:
    bool _running = true;
}};
"""

    cpp_content = f"""#include "{name}Game.h"
#include "GameRegistry.h"

void {name}Game::onEnter(Console& ctx) {{
    _running = true;
}}

void {name}Game::onExit(Console& ctx) {{
}}

void {name}Game::update(Console& ctx, float dt) {{
    if (ctx.justPressed(Btn::B)) {{
        _running = false;
    }}
}}

void {name}Game::draw(Console& ctx) {{
    ctx.setFont(u8g2_font_6x10_tf);
    ctx.drawStr(10, 30, "{name} Game");
}}

bool {name}Game::isRunning() const {{
    return _running;
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