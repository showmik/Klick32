#pragma once
// Target path: lib/Core/Scene.h
#include "Console.h"

// ─── Scene ────────────────────────────────────────────────────────────────────
// Abstract base for a single screen inside a multi-screen game.
//
// Lifecycle (managed by SceneManager):
//   onEnter(ctx)          — called once when this scene becomes active.
//                           Load per-screen state here (reset timers, etc.).
//   update(ctx, sm)       — called every frame; advance logic and read input.
//                           sm is the owning SceneManager — call sm.push(),
//                           sm.pop(), sm.replace(), or sm.clear() to transition.
//   draw(ctx)             — called every frame; render the current screen.
//                           The OS clears and flushes the buffer around this.
//   onExit(ctx)           — called once when this scene is popped or replaced.
//
// Rules:
//   • Scenes never store a Console& between frames — receive it per call only.
//   • Scenes own their visual state but NOT persistent data; use ctx.save*()
//     for anything that must survive a power cycle.
//   • A scene may call another scene's draw() to render a background layer
//     (e.g. PauseScene calls PlayScene::draw first, then overlays its box).
//   • Scenes are value members of the game class — no heap allocation needed.
//
// Typical pattern (inside update):
//
//   void TitleScene::update(Console& ctx, SceneManager& sm) {
//       if (ctx.justPressed(Btn::A))
//           sm.replace(_play, ctx);   // go to play screen
//   }
// ─────────────────────────────────────────────────────────────────────────────

class SceneManager; // forward

class Scene {
public:
    virtual ~Scene() = default;

    virtual void onEnter(Console& ctx) {}
    virtual void onExit (Console& ctx) {}

    // Called when the OS requests a snapshot save/load.
    virtual void saveSnapshot(Console& ctx) {}
    virtual void loadSnapshot(Console& ctx) {}
    virtual void onSnapshotRestored(Console& ctx) {}

    // Called every frame while this scene is on top of the stack.
    // Use sm to trigger transitions; use ctx for input, sound, and drawing.
    virtual void update(Console& ctx, SceneManager& sm, float dt) = 0;
    virtual void draw  (Console& ctx) = 0;

    // Override to return false when the screen hasn't changed, saving a
    // clearBuffer/sendBuffer cycle.  Defaults to true (always redraw).
    virtual bool needsRedraw() const { return true; }

    void setManager(SceneManager* sm) { _sm = sm; }

protected:
    SceneManager* _sm = nullptr;
};