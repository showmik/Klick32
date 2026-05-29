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
11. [Mathematics & Geometry (Vec2, Rect, and Helpers)](#11-mathematics--geometry-vec2-rect-and-helpers)
12. [Physics Engine (Swept-AABB Collision)](#12-physics-engine-swept-aabb-collision)
13. [Easing, Timers & Tweening](#13-easing-timers--tweening)
14. [UI Helper Widgets](#14-ui-helper-widgets)

**Part 4: Optimization & Tooling**
15. [Performance & Best Practices](#15-performance--best-practices)
16. [The PC Simulator Environment](#16-the-pc-simulator-environment)

**Part 5: Putting It All Together**
17. [Full End-to-End Example: Flappy Block](#17-full-end-to-end-example-flappy-block)

---

# Part 1: The Fundamentals

## 1. Core Philosophy & Hardware Constraints

Klick32 runs on an ESP32-S3. While powerful for a microcontroller, it has strict limitations compared to traditional PC game engines:

* **No Dynamic Allocation**: Heap fragmentation is fatal on microcontrollers. You must **never** use `new`, `delete`, or `std::vector` during gameplay. All memory must be pre-allocated globally or placed on the stack.
* **Monochrome Display**: The screen is 128x64 pixels and strictly 1-bit monochrome (Black or White). To achieve grayscale, Klick32 uses high-speed spatial dithering (checkerboard patterns).
* **Frame Budget**: The OS targets 30 FPS. You have approximately `33.3ms` per frame to read input, update physics, and push 8,192 pixels over the I2C bus.

---

## 2. Getting Started (Scaffolding)

To create a new game, use the provided Python script in the root directory:

```bash
python create_game.py MyCoolGame
```

This generates a folder in `lib/MyCoolGameGame/` containing your game's boilerplate headers and source files. The OS automatically discovers this folder during compilation via the `pre_build.py` script, includes its main header in `src/GameIncludes.h`, and registers it in the System Menu.

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

### 5.1 Input (Buttons)
Read inputs via the `ctx` object in `update()`:
* `ctx.pressed(Btn::A)` — true as long as held (debounced). Use for continuous actions (e.g., walking, charging).
* `ctx.justPressed(Btn::A)` — true only on the single frame the button transitions unpressed → pressed. Use for one-shots (e.g., jumping, firing, confirming).
* `ctx.justReleased(Btn::A)` — true only on the single frame the button transitions pressed → unpressed.
* `ctx.repeat(Btn::UP)` — true on justPressed, and then fires periodically while held down. Use for UI/menu navigation.
* `ctx.holdFrames(Btn::A)` — returns the consecutive number of frames the button has been held down. Resets to 0 on release. Useful for charge mechanics (e.g. `if (ctx.holdFrames(Btn::A) == 60) { /* fully charged */ }`).

Available buttons: `Btn::UP`, `Btn::DOWN`, `Btn::LEFT`, `Btn::RIGHT`, `Btn::A`, `Btn::B`, `Btn::MENU1`, `Btn::MENU2`.

> **Note on MENU2:** The Klick32 OS intercepts `MENU2` to open the universal **Quick Settings** overlay (which pauses the game and allows muting audio or quitting). You should avoid using `MENU2` for in-game actions, as it will physically suspend your game loop!
> Holding `MENU1` + `MENU2` toggles the Developer Diagnostics HUD.

### 5.2 Drawing
All drawing is automatically translated by the Camera system (see Section 10).

* **Primitives**: `ctx.drawBox(x, y, w, h)`, `ctx.drawFrame(x, y, w, h)`, `ctx.drawCircle(x, y, r)`.
* **Colors**: `ctx.setDrawColor(Console::COLOR_WHITE)` or `Console::COLOR_BLACK`. You can also use `Console::COLOR_XOR` to invert pixels beneath the shape.
* **Spatial Dithering**: Since we lack true grayscale, you can draw shaded regions using `drawDitherBox(x, y, w, h, shade)`. Shade goes from 0 (Black) to 1 (25%), 2 (50%), 3 (75%), and 4 (White).
* **Text**: `ctx.setFont(u8g2_font_...)`, then `ctx.drawStr(x, y, "Text")`. Remember that `y` is the **baseline** of the font, not the top! Center text easily using `ctx.drawStrCentered(y, "Text")` or centered both horizontally and vertically using `ctx.drawStrCenteredBoth("Text")`.

### 5.3 Bitmaps & Flipping
You can draw PROGMEM bitmaps. The `drawBitmapEx` function natively supports hardware-accelerated rendering and software flipping.

```cpp
// Normal draw
ctx.drawBitmap(x, y, bytesPerRow, height, spriteData);

// Flipped draw (Flips horizontally across the Y axis)
ctx.drawBitmapEx(x, y, width, height, bytesPerRow, spriteData, Console::BMP_FLIP_H);
```

For a cleaner syntax that avoids positional argument confusion, use the fluent **Bitmap Builder**:
```cpp
ctx.blit(spriteData, bytesPerRow, height)
   .at(x, y)
   .flipX(true) // horizontally flipped!
   .draw();
```

### 5.4 Audio
The Klick32 uses a polyphonic software synthesizer running asynchronously on **Core 0**. This means audio mixing happens completely independently from your game logic (which runs on Core 1).

### Sound Effects
Use the built-in system effects for consistency:
```cpp
ctx.sfxJump();
ctx.sfxDeath();
ctx.sfxPoint();
ctx.sfxMenuNav();
ctx.sfxMenuEnter();
```
Or trigger a custom beep:
```cpp
ctx.beep(440, 100); // 440Hz for 100ms
```

### Background Music (Multi-Threaded Tracker)
You can compose multi-track chiptune music by creating arrays of `ToneStep`. The synthesizer will play them in the background without dropping your game's framerate!

```cpp
// 1. Define your track (End with 0 duration to stop)
const ToneStep level1_BGM[] = {
    { 261, 200, Waveform::SQUARE },   // C4
    { 329, 200, Waveform::SQUARE },   // E4
    { 392, 400, Waveform::TRIANGLE }, // G4
    { 0, 0, Waveform::SQUARE }        // End of sequence
};

// 2. Play it in your onEnter hook
void MyGame::onEnter(Console& ctx) {
    ctx.playTrack(level1_BGM);
}
```

### 5.5 Persistence (Saving Data)
Klick32 automatically sandboxes your game's save data into a secure NVS (Non-Volatile Storage) namespace. You can safely save keys like `"score"` without overwriting another game's `"score"`. Maximum length for NVS keys is 15 characters.

```cpp
// Write methods:
ctx.saveUInt("hi", 1234);
ctx.saveInt("level", -5);
ctx.saveFloat("vel", 3.14f);
ctx.saveBool("sfx", true);
ctx.saveByte("id", 0xFA);

// Read methods (returns default value if key doesn't exist):
uint32_t val = ctx.loadUInt("hi", 0);
int32_t level = ctx.loadInt("level", 1);
float vel = ctx.loadFloat("vel", 0.0f);
bool sfx = ctx.loadBool("sfx", true);
uint8_t id = ctx.loadByte("id", 0);
```

#### 🏆 High Score Persistence Shortcuts
Rather than manually validating and saving scores, the OS provides highly-optimized shortcuts:
```cpp
// Loads the high score for this game (defaults to 0)
uint32_t best = ctx.loadHiScore();

// Compares, updates NVS *only* if beat, and returns true.
// Ideal for checking for a new record and playing a fanfare!
if (ctx.updateHiScore(score)) {
    ctx.sfxPoint(); // Play sound effect
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
   * Code output format: `Arduino Code`
   * Draw mode: `Horizontal, 1 bit per pixel`
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

## 8. Zero-Allocation ECS (Entity Component System)

Because `std::vector` is forbidden, Klick32 provides a lightweight, SoA (Structure-of-Arrays) ECS designed specifically for microcontrollers. It completely replaces OOP inheritance (e.g., `class Bullet : public Entity`) with pure, contiguous data arrays for maximum cache locality and physics performance.

```cpp
#include "ECS.h"

// 1. Define your components as simple structs
struct Transform { float x, y; };
struct Physics   { float vx, vy; };

// 2. Instantiate your registry and component pools
ECSRegistry<32> _registry;
ComponentPool<Transform, 32> _transforms;
ComponentPool<Physics, 32> _physics;

// 3. Spawn entities
EntityID e = _registry.create();
if (e != INVALID_ENTITY) {
    _transforms.add(e, {playerX, playerY});
    _physics.add(e, {50.0f, 0.0f}); // Move right 50 pixels per sec
}

// 4. Update via system loops (Fast contiguous array iteration!)
for (EntityID i = 0; i < 32; i++) {
    if (_registry.isValid(i) && _transforms.has[i] && _physics.has[i]) {
        _transforms.data[i].x += _physics.data[i].vx * dt;
        
        if (_transforms.data[i].x > 128) {
            _registry.destroy(i);
        }
    }
}
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

## 11. Mathematics & Geometry (Vec2, Rect, and Helpers)

Klick32 bundles header-only, zero-allocation math utilities in `<GameUtils.h>` to simplify 2D operations.

### 11.1 Vec2 (2D Floating-Point Vector)
Represent coordinates, speeds, and dimensions with `Vec2`.
```cpp
#include "GameUtils.h"

Vec2 pos{10.0f, 36.0f};
Vec2 vel{0.0f, -8.0f};

// Vector arithmetic
pos += vel * dt;

// Length queries
float lenSq = vel.lengthSq(); // Squared length (fast - avoids sqrtf)
float len = vel.length();     // Real Euclidean length

// Manhattan distance (absolute grid distance)
float distGrid = vel.manhattan(); 

// Normalization & Clamping
Vec2 dir = vel.normalized(); // Returns unit vector
Vec2 bounded = pos.clamped({0, 0}, {128, 64});

// Convert to integer screen-space
int screenX = pos.ix();
int screenY = pos.iy();
```

### 11.2 Rect (Integer AABB)
Represent hitboxes, screen boundaries, and check collisions.
```cpp
// Construct: x, y, width, height
Rect playerBox{dinoX + 4, (int)dinoY + 2, 8, 12};
Rect obstacleBox{obsX, obsY, 6, 8};

// AABB Intersection check
if (playerBox.overlaps(obstacleBox)) {
    // Boom!
}

// Point containment check
if (playerBox.contains(mouseX, mouseY)) { /* ... */ }

// Get center
Vec2 center = playerBox.center();

// Inset/Shrink hitboxes (dx narrower on each side, dy shorter on each top/bottom)
Rect hitbox = playerBox.inset(2, 1); 
```

### 11.3 Core Math Utilities
Common functions prefixed with `g` to avoid standard namespace conflicts:
* `gclamp(v, lo, hi)`: Clamps value `v` into closed interval `[lo, hi]`.
* `gsign(v)`: Returns `-1` if `v < 0`, `0` if `v == 0`, and `+1` if `v > 0`.
* `lerpf(a, b, t)`: Linearly interpolates from `a` to `b` based on float `t` `[0.0, 1.0]`.
* `lerpi(a, b, t, tmax)`: Precise integer lerp. Returns `a + (b - a) * t / tmax`. Excellent for UI slide-in animations.
* `mapRange(v, inMin, inMax, outMin, outMax)`: Maps a value from input range to output range.
* `wrapIdx(idx, count)`: Cyclic index wrapper that safely maps negative index offsets (D-pad LEFT/RIGHT UI menus).

---

## 12. Physics Engine (Swept-AABB Collision)

To prevent high-speed projectiles or falling players from bypassing static hitboxes (known as tunneling), Klick32 offers Swept-AABB collision resolution under `<PhysicsEngine.h>`.

```cpp
#include "PhysicsEngine.h"

Rect bulletBox{bulletX, bulletY, 2, 2};
Vec2 velocity{100.0f, 0.0f}; // Moving at 100 pixels per frame!
Rect brickBox{80, bulletY - 2, 10, 10};

// Swept AABB returns if and when a collision occurs during this frame
RaycastResult result = PhysicsEngine::sweptAABB(bulletBox, velocity, brickBox);

if (result.hit) {
    // result.time is [0.0, 1.0] indicating exactly where along velocity it hit.
    float hitX = bulletBox.x + velocity.x * result.time;
    
    // Surface normal allows bouncing:
    if (result.normalX != 0) velocity.x *= -1.0f; // Bounce X
    if (result.normalY != 0) velocity.y *= -1.0f; // Bounce Y
}
```

---

## 13. Easing, Timers & Tweening

Animate menu panels, player movement, and score rolls smoothly without manual millisecond tracking.

### 13.1 Timer
Replaces messy `millis()` variables with a single tracking instance.
```cpp
#include "Timer.h"

Timer _shootTimer;

// One-shot timer: 500ms
_shootTimer.start(500);

// Repeating timer: ticks every 1000ms
_shootTimer.startRepeating(1000);

// In update():
if (_shootTimer.tick()) {
    // Fires once (or every 1sec if repeating)
}

// Get completion progress [0.0f, 1.0f]
float progress = _shootTimer.progress();
```

### 13.2 Easing & Tweening
The `Tween` template class manages starting, interpolating, and retrieving values over time using custom curves.
```cpp
#include "Tween.h"

Tween<float> _uiSlideX{-128.0f}; // Initialize at -128.0f

// Tween to 0.0f over 800 milliseconds using quadratic ease out curve
_uiSlideX.start(-128.0f, 0.0f, 800, Ease::outQuad);

// In update():
_uiSlideX.update(); // Tick tween progress

// In draw():
ctx.drawBox(_uiSlideX.val(), 10, 50, 20); // Animates sliding in!
```

Available ease functions in the `Ease` namespace:
* `Ease::linear`
* `Ease::inQuad` / `Ease::outQuad` / `Ease::inOutQuad`
* `Ease::inCubic` / `Ease::outCubic` / `Ease::inOutCubic`
* `Ease::outBounce` (Bouncy fallbacks)
* `Ease::outElastic` (Wobbling spring effect)

---

## 14. UI Helper Widgets

Klick32 provides a reusable vertical layout, selection system, and widgets in the `UI` namespace to avoid writing boilerplate menus.

```cpp
#include "UI.h"

// 1. Progress Bar
// Draws an AABB border box and fills it relative to progress [0.0, 1.0]
UI::drawBar(ctx, 10, 10, 100, 6, healthPercentage);

// 2. Score Widget
// Renders a tiny label "SCORE" and formatting for high scores below it
UI::drawScore(ctx, 10, 25, currentScore, "BEST");

// 3. Vertical Text Menu (updates selection automatically and renders cursor)
const char* menuItems[] = {"RESUME", "RESTART", "QUIT"};
uint8_t selectedIndex = 0; // State variable (store in your class)

// Call this every frame inside title or pause loops. 
// Automatically handles Btn::UP, Btn::DOWN input and plays nav audio cues.
// Returns true when the player confirms with Btn::A.
if (UI::updateAndDrawMenu(ctx, menuItems, 3, selectedIndex, 20)) {
    if (selectedIndex == 0) resumeGame();
    if (selectedIndex == 2) exitGame();
}
```

---

# Part 4: Optimization & Tooling

## 15. Performance & Best Practices

1. **Avoid floating point math** where integers will suffice. (e.g. `millis()` timing vs `dt` tracking). The FPU is fast, but integer math is faster.
2. **Minimize `drawPixel` loops.** If you need to fill an area with a pattern, rely on `drawDitherBox` which is heavily optimized at the OS level.
3. **Use the Diagnostics HUD.** While your game is running, press `MENU2` (or the mapped button if your game overrides it) to bring up the Diagnostics overlay. Keep an eye on your "Logic Time". You have `33,000 µs` total; try to keep game logic under `15,000 µs` to leave enough time for I2C display flushing.
4. **Use Fixed Length Types:** Always use `int8_t`, `uint16_t`, `uint32_t` etc. instead of `int` or `long` to ensure precise memory footprints across hardware architectures.
5. **Dynamic Power Scaling:** If your game is simple (e.g., a puzzle game), you can drop the CPU speed from 240MHz down to 80MHz to drastically save battery life by calling `ctx.setCPUSpeed(80);` in `onEnter()`. The OS will automatically restore 240MHz when your game exits.

---

## 16. The PC Simulator Environment

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

## 17. Full End-to-End Example: Flappy Block

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
        _d.hiScore = ctx.loadHiScore(); // Load saved score
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
            if (ctx.updateHiScore(_d.score)) {
                ctx.sfxPoint(); // Play fanfare sound!
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
