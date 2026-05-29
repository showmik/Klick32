#pragma once
#include "Console.h"

// ─── GameBase ─────────────────────────────────────────────────────────────────
// Every game inherits from this class and implements all pure-virtual methods.
//
// Lifecycle managed by the OS:
//   1. OS opens the NVS namespace for this game.
//   2. OS calls onEnter(ctx)                   — game loads saved data and resets state.
//   3. OS calls update(ctx) + draw(ctx) every frame while the game is active.
//   4. When isRunning() returns false:
//        OS calls onExit(ctx)                  — game flushes any remaining saves.
//        OS closes the NVS namespace.
//        OS returns to the menu.
//
// Frame contract:
//   update(ctx) — advance logic, read input, trigger sounds.
//   draw(ctx)   — render to the display buffer.
//               The OS calls clearBuffer() before and sendBuffer() after;
//               games never call either themselves.
//
// Both methods receive a Console& — the single context object that wraps
// the display, input, sound, and save systems.  Games never include U8g2lib.h,
// InputManager.h, Sound.h, or SaveManager.h directly.
//
// Save / load pattern:
//   onEnter → ctx.loadHiScore() / ctx.loadUInt("level") / etc.
//   onExit  → ctx.saveHiScore(_hi) / ctx.saveUInt("level", _lvl) / etc.
//   On death (mid-session) → ctx.saveHiScore(_hi) guards against power loss.
//
// ⚠ STATIC SINGLETON WARNING:
//   REGISTER_GAME() creates a `static` local instance.  Your game object
//   SURVIVES between launches — member variables are NOT re-initialized.
//   onEnter() MUST explicitly reset ALL game state (scores, entities,
//   timers, flags).  Any member left untouched retains its value from the
//   previous play session.
//
// To exit back to the menu: set your internal _running flag to false.
// The OS detects it via isRunning(), calls onExit(ctx), then returns to menu.
// ─────────────────────────────────────────────────────────────────────────────
class GameBase {
public:
    virtual ~GameBase() = default;

    // Called once when the game is launched from the menu.
    // NVS namespace is already open — safe to call ctx.load*() here.
    virtual void onEnter(Console& ctx) = 0;

    // Called once when the game exits back to the menu.
    // NVS namespace is still open — safe to call ctx.save*() here.
    virtual void onExit(Console& ctx) = 0;

    // Called when the user triggers a "Save Snapshot" from the OS quick menu.
    // Games should serialize their current state to NVS here.
    virtual void saveSnapshot(Console& ctx) {}
    
    // Called when the user triggers a "Load Snapshot" from the OS quick menu.
    // Games should deserialize their state from NVS here.
    virtual void loadSnapshot(Console& ctx) {}

    // Called every frame: advance game logic, read input, trigger sounds.
    virtual void update(Console& ctx, float dt) = 0;

    // Called every frame after update: render the current frame.
    virtual void draw(Console& ctx)   = 0;

    // Returns true if the screen needs to be redrawn this frame. 
    // Defaults to true for standard games, but UI elements can override it.
    virtual bool needsRedraw() const { return true; }

    // Returns false when the game wants to exit back to the menu.
    virtual bool isRunning() const = 0;

    // Short display name shown in the OS menu (max ~12 chars).
    // Also used as the NVS namespace key — keep it stable across firmware updates.
    virtual const char* getName() const = 0;

    // Optional 16×16 PROGMEM icon bitmap for the menu card.
    // Return nullptr to use the OS placeholder (initial letter in a box).
    virtual const uint8_t* getIcon() const { return nullptr; }

    // Optional 128x45 PROGMEM cover art bitmap for the menu card.
    // Dimensions: 128px wide (16 bytes per row) x 45px high.
    // Return nullptr to fall back to the standard icon.
    virtual const uint8_t* getCoverArt() const { return nullptr; }
};