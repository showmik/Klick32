import os
import glob
import re

dirs = ['DinoGame', 'PongGame', 'SnakeGame', 'SpaceInvaders', 'TinyRogue']
files = []
for d in dirs:
    path = os.path.join('lib', d)
    files.extend(glob.glob(os.path.join(path, '**', '*.h'), recursive=True))
    files.extend(glob.glob(os.path.join(path, '**', '*.cpp'), recursive=True))

for f in files:
    with open(f, 'r', encoding='utf-8') as file:
        content = file.read()
        
    orig_content = content
    
    # Headers
    content = re.sub(r'(void\s+update\s*\(\s*Console&\s*ctx\s*,\s*SceneManager&\s*sm)(\s*\))', r'\1, float dt\2', content)
    content = re.sub(r'(void\s+update\s*\(\s*Console&\s*ctx)(\s*\))', r'\1, float dt\2', content)
    
    # CPPs
    content = re.sub(r'(void\s+\w+::update\s*\(\s*Console&\s*ctx\s*,\s*SceneManager&\s*sm)(\s*\))\s*\{', r'\1, float dt\2 {', content)
    content = re.sub(r'(void\s+\w+::update\s*\(\s*Console&\s*ctx)(\s*\))\s*\{', r'\1, float dt\2 {', content)

    # GameRegistry & REGISTER_GAME
    if f.endswith('.cpp'):
        game_class = os.path.basename(f).replace('.cpp', '')
        if game_class in ['DinoGame', 'PongGame', 'SnakeGame', 'SpaceInvadersGame', 'TinyRogueGame']:
            if '#include "GameRegistry.h"' not in content:
                content = re.sub(r'(#include\s+"[^"]+")', r'\1\n#include "GameRegistry.h"', content, count=1)
            
            reg_str = f"REGISTER_GAME({game_class});"
            if reg_str not in content:
                content = content.strip() + f"\n\n{reg_str}\n"

    if content != orig_content:
        with open(f, 'w', encoding='utf-8') as file:
            file.write(content)
        print(f"Updated {f}")
