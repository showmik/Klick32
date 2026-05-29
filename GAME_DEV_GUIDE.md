# Klick32 Developer Guide

Welcome to the Klick32 OS! This guide will walk you through the process of creating a new game, understanding the core engine features, and building for the ESP32-S3 hardware.

---

## 1. Creating a New Game

The easiest way to start is by using the scaffolding script. From the root directory, run:

```bash
python scripts/create_game.py "My Cool Game"
```

This will:
1. Create a `lib/MyCoolGame/` directory.
2. Generate the boilerplate `MyCoolGame.h` and `MyCoolGame.cpp`.
3. The next time you build, the `pre_build.py` script will automatically find your game and register it in the OS menu!

## 2. The Game Architecture

Klick32 uses a static singleton pattern for games to avoid dynamic memory allocation (heap fragmentation is fatal on ESP32).

Your game inherits from `SceneGame<TShared>`, which provides a `SceneManager`, `Camera`, `ParticleManager`, and a shared data struct automatically.

### ⚠️ The Static Singleton Hazard
Because your game is a static instance, **it survives between launches.** 
You MUST explicitly reset all your game state inside `onEnter(Console& ctx)`.

```cpp
void MyCoolGame::onEnter(Console& ctx) {
    // 🔴 BAD: Leaving score untouched means the player starts with their 
    //         previous score from the last time they played!
    
    // 🟢 GOOD: Explicitly zero everything out.
    _data.score = 0;
    _data.lives = 3;
    _particles.clear();
    
    // Wire up pause and game over scenes
    useDefaultEvents(&_pauseScene, &_gameOverScene);
    _sm.replace(&_titleScene, ctx);
}
```

## 3. Scenes

A `Scene` is a self-contained state (like Title, Play, Pause). 

```cpp
void TitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::A)) {
        sm.replace(playScene, ctx, SceneManager::Effect::FADE);
    }
}
```

The `SceneManager` handles transitions. The most common are:
* `push()` - Opens an overlay (e.g. Pause Menu)
* `pop()` - Closes an overlay
* `replace()` - Hard cuts to a new scene (e.g. Title -> Gameplay)

## 4. The Console (HAL)

The `Console` object (`ctx`) is passed to every `update()` and `draw()` call. It wraps input, display, audio, and save data.

### Input
```cpp
if (ctx.justPressed(Btn::UP)) { ... }
if (ctx.repeat(Btn::LEFT)) { ... } // Fires repeatedly if held
```

### Drawing
Klick32 uses U8G2 internally. The console handles coordinate offsets automatically via the `Camera`.
```cpp
ctx.setDrawColor(Console::COLOR_WHITE);
ctx.drawBox(x, y, width, height);

// Draw a spatial dither pattern (0=Black, 1=25%, 2=50%, 3=75%, 4=White)
ctx.drawDitherBox(0, 0, 128, 64, 2); 
```

### Audio
```cpp
ctx.sfxJump();       // Pre-defined SFX
ctx.beep(440, 100);  // Custom tone: 440Hz for 100ms
```

### Saving & Loading
The OS automatically manages namespaces. You can safely save and load from within your game, and it won't overwrite other games.
```cpp
uint32_t highscore = ctx.loadUInt("hi", 0);
ctx.saveUInt("hi", highscore + 100);
```

## 5. Performance Tips

* **Use the Entity Pool:** Never use `std::vector` or `new`/`delete` during gameplay. Use the fixed `EntityManager` pool.
* **Avoid floats:** The ESP32-S3 has an FPU, but integer math is still faster. Use floats only when strictly necessary (e.g. precise physics).
* **Use drawDitherBox over pixels:** If you need a transparent shade, use `ctx.drawDitherBox` instead of looping `drawPixel`.
* **Diagnostics:** Press `MENU2` while your game is running to toggle the Diagnostics overlay. Keep your logic under 15,000 µs to maintain 30 FPS.
