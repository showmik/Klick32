#pragma once
#include "GameBase.h"
#include "SceneManager.h"
#include "Camera.h"
#include "ParticleManager.h"

// ─── SceneGame ───────────────────────────────────────────────────────────────
// A boilerplate-reducing template for games that use a SceneManager, Camera,
// and ParticleManager.
//
// Most Klick32 games share the exact same structural boilerplate:
//   - A struct holding shared game state (TShared).
//   - A SceneManager, Camera, and ParticleManager.
//   - The same update/draw loop that ticks the camera, particles, and scenes.
//   - The same onExit/isRunning logic based on the SceneManager stack.
//   - The same standard event bindings (PAUSE -> push pause scene, etc).
//
// Inheriting from SceneGame<MySharedData> handles all this automatically.
// Games only need to implement:
//   - getName(), getIcon(), getCoverArt()
//   - onEnter(ctx) -> wire scene dependencies and push the initial scene.
// ─────────────────────────────────────────────────────────────────────────────

template <typename TShared>
class SceneGame : public GameBase {
public:
    TShared         _data;
    SceneManager    _sm;
    Camera          _camera;
    ParticleManager _particles;

    // Registers standard system events to common scene transitions.
    // Pass nullptr if your game doesn't use a specific scene.
    void useDefaultEvents(Scene* pauseScene, Scene* gameOverScene) {
        _sm.clearTransitions();
        _sm.onEvent(Event::QUIT, SceneManager::CLEAR);
        
        if (pauseScene) {
            _sm.onEvent(Event::PAUSE,  SceneManager::PUSH, pauseScene);
            _sm.onEvent(Event::RESUME, SceneManager::POP);
        }
        
        if (gameOverScene) {
            _sm.onEvent(Event::GAME_OVER, SceneManager::REPLACE, gameOverScene);
        }
    }

    // Default implementations for standard GameBase methods
    
    void onExit(Console& ctx) override {
        // SceneManager::clear automatically calls onExit() on any active scenes
        _sm.clear(ctx);
    }

    void update(Console& ctx, float dt) override {
        _camera.update(dt);
        _particles.update(dt);
        _sm.update(ctx, dt);
    }

    void draw(Console& ctx) override {
        _sm.draw(ctx);
    }

    bool isRunning() const override {
        return !_sm.empty();
    }
};
