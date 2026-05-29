# Klick32 Developer Guide: The Complete Manual

Welcome to the Klick32 OS Developer Guide! Klick32 is a custom game console operating system built for the ESP32-S3 microcontroller, featuring a 128x64 monochrome OLED display (SH1106). 

This guide provides a deep dive into the architecture, limitations, and APIs of the Klick32 engine. By the end of this guide, you will understand how to build highly optimized, polished, and memory-safe games for the hardware.

---

## Table of Contents

**Part 1: The Fundamentals**
1. [Core Philosophy & Hardware Constraints](#1-core-philosophy--hardware-constraints)
2. [Getting Started (Scaffolding)](#2-getting-started-scaffolding)
3. [The Static Singleton Architecture](#3-the-static-singleton-architecture)

**Part 2: Core APIs & Assets**
4. [Scene Management](#4-scene-management)
5. [The Console Hardware Abstraction Layer (HAL)](#5-the-console-hardware-abstraction-layer-hal)
6. [Asset Pipeline (Bitmaps)](#6-asset-pipeline-bitmaps)
7. [Game Cards & Cover Art](#7-game-cards--cover-art)

**Part 3: Advanced Systems**
8. [Zero-Allocation Entity Management](#8-zero-allocation-entity-management)
9. [Particle System](#9-particle-system)
10. [Camera & Screen Shake](#10-camera--screen-shake)

**Part 4: Optimization & Tooling**
11. [Performance & Best Practices](#11-performance--best-practices)
12. [The PC Simulator Environment](#12-the-pc-simulator-environment)

**Part 5: Putting It All Together**
13. [Full End-to-End Example: Flappy Block](#13-full-end-to-end-example-flappy-block)

---

# Part 1: The Fundamentals

## 1. Core Philosophy & Hardware Constraints

Klick32 runs on an ESP32-S3. While powerful for a microcontroller, it has strict limitations compared to traditional PC game engines:

* **No Dynamic Allocation**: Heap fragmentation is fatal on microcontrollers. You must **never** use `new`, `delete`, or `std::vector` during gameplay. All memory must be pre-allocated globally or placed on the stack.
* **Monochrome Display**: The screen is 128x64 pixels and strictly 1-bit monochrome (Black or White). To achieve grayscale, Klick32 uses high-speed spatial dithering (checkerboard patterns).
* **Frame Budget**: The OS targets 30 FPS. You have approximately `33.3ms` per frame to read input, update physics, and push 8,192 pixels over the I2C bus.

---

## 2. Getting Started (Scaffolding)

To create a new game, use the provided Python script. From the root directory, run:

```bash
python scripts/create_game.py "My Cool Game"
```

This generates a folder in `lib/MyCoolGame/` containing your game's boilerplate headers and source files. The OS automatically discovers this folder during compilation via the `pre_build.py` script and registers it in the System Menu.

---

## 3. The Static Singleton Architecture

To comply with the "No Dynamic Allocation" rule, games in Klick32 are instantiated once at boot time as **Static Singletons**.

When the OS boots, it calls a macro in your game's source file:
```cpp
REGISTER_GAME(MyCoolGame)
```
This creates a single, permanent instance of your game in memory. 

### ⚠️ The Static Hazard (CRITICAL)
Because your game object survives indefinitely, **state is not cleared when the player exits the game.** If a player gets a Game Over, exits to the OS Menu, and launches your game again, your class member variables will retain their exact values from the previous session!

You MUST manually reset all state in the `onEnter()` hook:

```cpp
void MyCoolGame::onEnter(Console& ctx) {
    // 🔴 BAD: Leaving score untouched means the player starts with 
    // their previous score!
    
    // 🟢 GOOD: Explicitly reset all gameplay variables
    _sharedData.score = 0;
    _sharedData.lives = 3;
    _sharedData.playerX = 10;
    
    // Clear all active entities and particles
    _entityManager.clear();
    _particles.clear();
    
    // Reset the scene stack to the Title Screen
    _sm.replace(&_titleScene, ctx);
}
```

---

# Part 2: Core APIs & Assets

## 4. Scene Management

Klick32 games are usually broken up into "Scenes" (e.g., `TitleScene`, `PlayScene`, `PauseScene`). The `SceneManager` is a stack-based router that handles transitioning between these states.

### Scene Transitions
Inside a scene's `update()` method, you transition using the `SceneManager`:

* `sm.push(Scene*, ctx)`: Pauses the current scene and pushes a new one on top. (Useful for Pause Menus or Inventory Overlays).
* `sm.pop(ctx)`: Closes the current top scene, resuming the one beneath it.
* `sm.replace(Scene*, ctx)`: Clears the entire stack and pushes the new scene. (Useful for hard transitions like Title -> Play).
* `sm.clear(ctx)`: Clears the stack. When the stack is empty, the game exits back to the OS Menu automatically.

### Transition Effects
You can avoid jarring hard-cuts by utilizing the built-in transition engine:
```cpp
// Fades the screen to black, swaps scenes, then fades back in
sm.replace(&playScene, ctx, SceneManager::Effect::FADE);
```

### Scene Draw Ordering
Scenes pushed on top of the stack block `update()` calls to scenes beneath them, but you can explicitly render underlying scenes to create overlays:

```cpp
void PauseScene::draw(Console& ctx) {
    // Render the frozen gameplay in the background
    _manager->drawUnder(ctx); 
    
    // Draw a semi-transparent dithered box over the game
    ctx.drawDitherBox(0, 0, 128, 64, 2); 
    
    // Draw "PAUSED" text
    ctx.drawStrCentered(32, "- PAUSED -");
}
```

---

## 5. The Console Hardware Abstraction Layer (HAL)

The `Console` object (passed as `ctx`) is your sole interface to the hardware. You never include `U8g2` or `Preferences` directly in your game files.

### 5.1 Input Polling
Read button states during `update()`:
* `ctx.pressed(Btn::A)`: Returns true every frame the button is held. Good for continuous movement (e.g., accelerating a car).
* `ctx.justPressed(Btn::A)`: Returns true for exactly one frame when the button is first pushed. Good for single actions (e.g., jumping, menu selection).
* `ctx.justReleased(Btn::A)`: Returns true when the button is let go.
* `ctx.repeat(Btn::UP)`: Returns true on press, and then pulses true repeatedly if held (ideal for navigating menus quickly).

Available buttons: `UP`, `DOWN`, `LEFT`, `RIGHT`, `A`, `B`, `MENU1`, `MENU2`.

### 5.2 Drawing
All drawing is automatically translated by the Camera system (see Section 10).

* **Primitives**: `ctx.drawBox(x, y, w, h)`, `ctx.drawFrame(x, y, w, h)`, `ctx.drawCircle(x, y, r)`.
* **Colors**: `ctx.setDrawColor(Console::COLOR_WHITE)` or `Console::COLOR_BLACK`. You can also use `Console::COLOR_XOR` to invert pixels beneath the shape.
* **Spatial Dithering**: Since we lack true grayscale, you can draw shaded regions using `drawDitherBox(x, y, w, h, shade)`. Shade goes from 0 (Black) to 1 (25%), 2 (50%), 3 (75%), and 4 (White).
* **Text**: `ctx.setFont(u8g2_font_...)`, then `ctx.drawStr(x, y, "Text")`. Remember that `y` is the **baseline** of the font, not the top!

### 5.3 Bitmaps & Flipping
You can draw PROGMEM bitmaps. The `drawBitmapEx` function natively supports hardware-accelerated rendering and software flipping.

```cpp
// Normal draw
ctx.drawBitmap(x, y, bytesPerRow, height, spriteData);

// Flipped draw (Flips horizontally across the Y axis)
ctx.drawBitmapEx(x, y, width, height, bytesPerRow, spriteData, Console::BMP_FLIP_H);
```

### 5.4 Audio
Use pre-defined SFX to ensure consistent audio across the OS. This prevents every game from having wildly different volume levels or abrasive tones.
```cpp
if (ctx.justPressed(Btn::A)) {
    playerVelocity = -10.0f; // Jump!
    ctx.sfxJump();           // Play OS-level jump sound
}
if (playerHitEnemy) {
    ctx.sfxDeath();
}
```
Or create custom tones: `ctx.beep(440, 100); // 440 Hz for 100ms`.

### 5.5 Persistence (Saving Data)
Klick32 automatically sandboxes your game's save data into a secure NVS (Non-Volatile Storage) namespace. You can safely save keys like `"score"` without overwriting another game's `"score"`.

A robust High Score system example:
```cpp
// 1. Load in onEnter()
void PlayScene::onEnter(Console& ctx) {
    _score = 0;
    _hiScore = ctx.loadUInt("hi", 0); // 0 is the default if not found
}

// 2. Check and Save mid-game when the player dies
void PlayScene::die(Console& ctx) {
    if (_score > _hiScore) {
        _hiScore = _score;
        ctx.saveUInt("hi", _hiScore); // Save immediately to protect against power-loss
    }
    _sm->replace(&_gameOverScene, ctx);
}
```

---

## 6. Asset Pipeline (Bitmaps)

Because Klick32 uses the `U8g2` display format, you cannot simply load `.png` or `.bmp` files. Graphics must be baked into your C++ code as `PROGMEM` byte arrays (XBM format).

**How to generate graphics:**
1. Draw your sprite in Aseprite, MS Paint, or any editor (using only strict Black and White pixels).
2. Export it as a `.bmp` or `.png`.
3. Use an online tool like **image2cpp** (https://javl.github.io/image2cpp/) or the U8g2 XBM tools.
4. Settings for `image2cpp`:
   - Code output format: `Arduino Code`
   - Draw mode: `Horizontal, 1 bit per pixel`
5. Copy the generated `const unsigned char [] PROGMEM` array into a header file (e.g. `assets.h`) and include it in your game.

> **Tip:** If your sprite has transparency, you will need to generate *two* bitmaps: the sprite data, and a separate "mask" data, drawing the mask first with `COLOR_BLACK` and the sprite with `COLOR_WHITE`.

---

## 7. Game Cards & Cover Art

By default, your game appears in the OS menu as a blank card with the first letter of its name. To make your game stand out, you can override two virtual functions in your `GameBase` class to provide custom artwork:

```cpp
// A tiny 16x16 icon displayed next to your game's name
const uint8_t* getIcon() const override {
    static const uint8_t PROGMEM icon[] = { ... };
    return icon;
}

// A massive 128x45 banner displayed when your game is selected
const uint8_t* getCoverArt() const override {
    static const uint8_t PROGMEM cover[] = { ... };
    return cover;
}
```

The cover art must be exactly 128 pixels wide (16 bytes per row) and 45 pixels high. The System Menu automatically handles drawing this beneath the header and above the footer!

---

# Part 3: Advanced Systems

## 8. Zero-Allocation Entity Management

Because `std::vector` is forbidden, Klick32 provides a templated `EntityManager`. It allocates a fixed block of memory at boot and recycles object slots dynamically.

```cpp
// 1. Define your entity
class Bullet : public Entity {
public:
    int x, y;
    void init(int startX, int startY) {
        x = startX;
        y = startY;
    }
    void update(Console& ctx, float dt) override {
        x += 50 * dt; // Move right 50 pixels per second
        if (x > 128) destroy(); // Returns this slot to the pool
    }
    void draw(Console& ctx) override {
        ctx.drawBox(x, y, 2, 2);
    }
};

// 2. Instantiate the manager in your Game class
EntityManager<Bullet, 32> _bullets; // Pool of 32 bullets

// 3. Spawn entities
Bullet* b = _bullets.spawn();
if (b) b->init(playerX, playerY); // Check for nullptr if pool is full!

// 4. Update and Draw
_bullets.update(ctx, dt);
_bullets.draw(ctx);
```

---

## 9. Particle System

The built-in `ParticleManager` is highly optimized for generating explosions, sparks, and debris.

```cpp
ParticleManager _particles;

// In update():
// Spawn 10 particles at (x,y), velocity ranging from -20 to 20, gravity 50, lifetime 1.5s
_particles.emit(10, x, y, -20, 20, -20, 20, 50, 1.5f);

_particles.update(ctx, dt);

// In draw():
_particles.draw(ctx);
```

---

## 10. Camera & Screen Shake

The OS features a global 2D Camera. By default, the camera is locked at `(0,0)`. Any drawing function you call on `Console` is automatically shifted by the Camera's current offset.

```cpp
Camera _camera;

// Make the camera track the player
_camera.setTarget(playerX - 64, playerY - 32);
_camera.update(dt); // Applies smooth lerping

// Add screen shake! (Intensity 5.0, dissipates by 10.0 per second)
_camera.shake(5.0f, 10.0f);

// Apply the camera to the console BEFORE drawing
_camera.apply(ctx);
```
To draw UI elements that shouldn't move (like a score counter), temporarily detach the camera:
```cpp
ctx.beginScreenSpace();
ctx.drawStr(0, 10, "Score: 100"); // Stays perfectly still
ctx.endScreenSpace();
```

---

# Part 4: Optimization & Tooling

## 11. Performance & Best Practices

1. **Avoid floating point math** where integers will suffice. (e.g. `millis()` timing vs `dt` tracking). The FPU is fast, but integer math is faster.
2. **Minimize `drawPixel` loops.** If you need to fill an area with a pattern, rely on `drawDitherBox` which is heavily optimized at the OS level.
3. **Use the Diagnostics HUD.** While your game is running, press `MENU2` (or the mapped button if your game overrides it) to bring up the Diagnostics overlay. Keep an eye on your "Logic Time". You have `33,000 µs` total; try to keep game logic under `15,000 µs` to leave enough time for I2C display flushing.
4. **Use Fixed Length Types:** Always use `int8_t`, `uint16_t`, `uint32_t` etc. instead of `int` or `long` to ensure precise memory footprints across hardware architectures.

---

## 12. The PC Simulator Environment

Flashing the ESP32-S3 over USB can take 30–60 seconds. When iterating on game logic, this is agonizingly slow. 

Klick32 includes a **Native PC Simulator**! 
Instead of compiling for the ESP32, you can compile the OS as a native Windows/Mac/Linux executable using CMake. 

To run the simulator:
1. Navigate to the `simulator/` directory.
2. Build using CMake: `cmake -S . -B build` then `cmake --build build`.
3. The simulator opens a window mapping your keyboard arrows to the D-Pad, `Z` to `A`, `X` to `B`, and `Enter`/`Shift` to the Menu buttons.

In your game code, you can use the `#ifdef SIMULATOR` block if you need to mock out hardware-specific logic that doesn't exist on the PC (like reading an analog sensor).

---

# Part 5: Putting It All Together

## 13. Full End-to-End Example: Flappy Block

The best way to learn is to see everything working together. Here is a complete, minimal implementation of a "Flappy Bird" clone using the Klick32 Engine.

Notice how the `GameBase` sets up the dependencies, while the `Scene` handles the actual logic.

```cpp
#pragma once
#include "SceneGame.h"

// ─── 1. Shared Data ───────────────────────────────────────────────────────────
// This struct holds data that needs to be accessed by multiple scenes.
struct FlappyData {
    float playerY;
    float velocity;
    uint32_t score;
    uint32_t hiScore;
};

// ─── 2. The Title Scene ───────────────────────────────────────────────────────
class FlappyTitle : public Scene {
    FlappyData& _d;
public:
    FlappyTitle(FlappyData& data) : _d(data) {}

    void onEnter(Console& ctx) override {
        // RESET STATE HERE! (The Static Singleton Rule)
        _d.playerY = 32.0f;
        _d.velocity = 0.0f;
        _d.score = 0;
        _d.hiScore = ctx.loadUInt("hi", 0); // Load saved score
    }

    void update(Console& ctx, SceneManager& sm, float dt) override {
        if (ctx.justPressed(Btn::A)) {
            // Hard transition to the play scene using a fade effect
            sm.replace(sm.getNextScene(), ctx, SceneManager::Effect::FADE);
            ctx.sfxMenuEnter();
        }
    }

    void draw(Console& ctx) override {
        ctx.setFont(u8g2_font_ncenB14_tr);
        ctx.drawStrCentered(30, "FLAPPY BLOCK");
        
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawStrCentered(50, "[A] to Flap");
        
        ctx.drawPrintf(2, 10, "HI: %lu", _d.hiScore);
    }
};

// ─── 3. The Play Scene ────────────────────────────────────────────────────────
class FlappyPlay : public Scene {
    FlappyData& _d;
public:
    FlappyPlay(FlappyData& data) : _d(data) {}

    void update(Console& ctx, SceneManager& sm, float dt) override {
        // Pause Menu overlay!
        if (ctx.justPressed(Btn::MENU1)) {
            sm.push(sm.getPauseScene(), ctx);
            return;
        }

        // Apply Gravity
        _d.velocity += 30.0f * dt; 
        
        // Flap!
        if (ctx.justPressed(Btn::A)) {
            _d.velocity = -15.0f;
            ctx.sfxJump();
        }

        _d.playerY += _d.velocity * dt;

        // Collision Check (Floor/Ceiling)
        if (_d.playerY > 60.0f || _d.playerY < 0.0f) {
            ctx.sfxDeath();
            if (_d.score > _d.hiScore) {
                ctx.saveUInt("hi", _d.score); // Save immediately
            }
            sm.replace(sm.getGameOverScene(), ctx, SceneManager::Effect::FADE);
        }
    }

    void draw(Console& ctx) override {
        // Draw the Player (a 4x4 block)
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawBox(10, (int)_d.playerY, 4, 4);

        // Draw HUD over everything using Screen Space
        ctx.beginScreenSpace();
        ctx.setFont(u8g2_font_5x7_tf);
        ctx.drawPrintf(110, 10, "%lu", _d.score);
        ctx.endScreenSpace();
    }
};

// ─── 4. The Main Game Class ───────────────────────────────────────────────────
// SceneGame automatically provides a SceneManager, Camera, and ParticleManager
class FlappyGame : public SceneGame<FlappyData> {
    FlappyTitle _title;
    FlappyPlay  _play;

public:
    FlappyGame() : _title(_data), _play(_data) {}

    const char* getName() const override { return "Flappy"; }

    void onEnter(Console& ctx) override {
        // Wire up the scenes. getNextScene() is a generic pointer we can 
        // use in the Title screen to know where to go next.
        _sm.setNextScene(&_play);
        
        // SceneGame provides default Pause and Game Over overlays
        useDefaultEvents(&_title, &_title); 

        // Start the game by loading the Title screen
        _sm.replace(&_title, ctx);
    }
};

// 5. Expose to the OS
REGISTER_GAME(FlappyGame)
```
