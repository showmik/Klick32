#pragma once
// Target path: lib/Core/SceneManager.h
#include "Scene.h"
#include "Event.h"

// ─── SceneManager ─────────────────────────────────────────────────────────────
// A small stack-based scene router used inside a single GameBase subclass.
//
// Stack semantics:
//   replace(s) — pop everything, push s.   Use for hard cuts: title → play.
//   push(s)    — push s on top.            Use for overlays: play → pause.
//   pop()      — remove top, reveal below. Use for closing overlays.
//   clear()    — pop everything.           Use when the game wants to exit:
//                                          empty stack → isRunning() false.
//
// Per-frame:
//   update() and draw() always operate on the TOP scene only.
//   needsRedraw() delegates to the top scene's flag.
//
// Lifecycle hooks (onEnter/onExit) are called automatically on every push/pop.
//
// Typical game shell:
//
//   class MyGame : public GameBase {
//       SceneManager _sm;
//       TitleScene _title;
//       PlayScene  _play;
//
//       void onEnter(Console& ctx) override { _sm.replace(&_title, ctx); }
//       void update (Console& ctx) override { _sm.update(ctx); }
//       void draw   (Console& ctx) override { _sm.draw(ctx); }
//       bool isRunning() const override { return !_sm.empty(); }
//   };
// ─────────────────────────────────────────────────────────────────────────────

class SceneManager {
public:
    static constexpr uint8_t MAX_DEPTH = 4;

    enum class Effect : uint8_t { NONE, FADE };

    // ── Transitions (safe to call from within Scene::update) ──────────────────

    // Hard-cut: exit all current scenes, enter s.
    void replace(Scene* s, Console& ctx, Effect effect = Effect::NONE) {
        if (effect != Effect::NONE) {
            _startEffect(effect, REPLACE, s);
            return;
        }
        while (_depth > 0) _stack[--_depth]->onExit(ctx);
        if (s) { _stack[_depth++] = s; s->setManager(this); s->onEnter(ctx); }
    }

    // Hard-cut directly into a scene bypassing onEnter(), used by snapshot system.
    void restoreSnapshotScene(Scene* s, Console& ctx) {
        while (_depth > 0) _stack[--_depth]->onExit(ctx);
        if (s) { _stack[_depth++] = s; s->setManager(this); s->onSnapshotRestored(ctx); }
    }

    // Overlay: pause the current top, push s on top.
    // Silently ignored if the stack is full.
    void push(Scene* s, Console& ctx, Effect effect = Effect::NONE) {
        if (!s || _depth >= MAX_DEPTH) return;
        if (effect != Effect::NONE) {
            _startEffect(effect, PUSH, s);
            return;
        }
        _stack[_depth++] = s;
        s->setManager(this);
        s->onEnter(ctx);
    }

    // Close overlay: exit top, resume the scene beneath.
    // Silently ignored if the stack is already empty.
    void pop(Console& ctx, Effect effect = Effect::NONE) {
        if (_depth == 0) return;
        if (effect != Effect::NONE) {
            _startEffect(effect, POP, nullptr);
            return;
        }
        _stack[--_depth]->onExit(ctx);
    }

    // Exit everything — game's isRunning() should check empty().
    void clear(Console& ctx, Effect effect = Effect::NONE) {
        if (effect != Effect::NONE) {
            _startEffect(effect, CLEAR, nullptr);
            return;
        }
        while (_depth > 0) _stack[--_depth]->onExit(ctx);
    }

    // ── Event Registry ────────────────────────────────────────────────────────

    enum Action : uint8_t { PUSH, REPLACE, POP, CLEAR };

    // Register a scene transition triggered by an event.
    void onEvent(Event::ID eventId, Action action, Scene* target = nullptr) {
        if (_numTransitions < MAX_TRANSITIONS) {
            _transitions[_numTransitions++] = {eventId, target, action};
        }
    }

    // Remove all registered event → transition mappings.
    // Call this before re-registering events (e.g., in onEnter) to prevent
    // stale or duplicate mappings when games are static singletons.
    void clearTransitions() { _numTransitions = 0; }

    // Emit an event to trigger registered transitions.
    void emit(Console& ctx, Event::ID eventId) {
        for (int i = 0; i < _numTransitions; i++) {
            if (_transitions[i].event == eventId) {
                switch (_transitions[i].action) {
                    case PUSH:    push(_transitions[i].target, ctx); break;
                    case REPLACE: replace(_transitions[i].target, ctx); break;
                    case POP:     pop(ctx); break;
                    case CLEAR:   clear(ctx); break;
                }
                return; // Handle only the first matching event map
            }
        }
    }

    // ── Per-frame dispatch ────────────────────────────────────────────────────

    void update(Console& ctx, float dt) {
        if (_effectActive) {
            _effectTimer--;
            // Halfway point: swap the scenes
            if (_effectTimer == _effectDuration / 2) {
                switch (_pendingAction) {
                    case PUSH:    push(_pendingScene, ctx); break;
                    case REPLACE: replace(_pendingScene, ctx); break;
                    case POP:     pop(ctx); break;
                    case CLEAR:   clear(ctx); break;
                }
            }
            if (_effectTimer == 0) {
                _effectActive = false;
            }
            return; // Pause logic during transition
        }

        if (_depth > 0) _stack[_depth - 1]->update(ctx, *this, dt);
    }

    void draw(Console& ctx) {
        if (_depth > 0) _stack[_depth - 1]->draw(ctx);
        
        if (_effectActive && _effect == Effect::FADE) {
            int half = _effectDuration / 2;
            int progress = (_effectTimer > half) 
                ? (_effectDuration - _effectTimer) // Fading out: 0 -> half
                : _effectTimer;                    // Fading in: half -> 0
                
            // Map 0..half to dither shade 0..5 (4 is solid, 5 gives a brief hold)
            int shade = (progress * 5) / half;
            if (shade > 4) shade = 4;
            
            ctx.pushDrawState();
            ctx.setDrawColor(Console::COLOR_BLACK);
            ctx.drawDitherBox(0, 0, Console::W, Console::H, shade);
            ctx.popDrawState();
        }
    }

    void drawUnder(Console& ctx) {
        if (_depth > 1) _stack[_depth - 2]->draw(ctx);
    }

    bool needsRedraw() const {
        if (_effectActive) return true;
        return (_depth > 0) && _stack[_depth - 1]->needsRedraw();
    }

    // ── Queries ───────────────────────────────────────────────────────────────

    Scene*  current() const { return (_depth > 0) ? _stack[_depth - 1] : nullptr; }
    uint8_t depth()   const { return _depth; }
    bool    empty()   const { return _depth == 0; }

private:
    Scene*  _stack[MAX_DEPTH] = {};
    uint8_t _depth             = 0;

    struct Transition {
        Event::ID event;
        Scene* target;
        Action action;
    };
    static constexpr uint8_t MAX_TRANSITIONS = 16;
    Transition _transitions[MAX_TRANSITIONS] = {};
    uint8_t _numTransitions = 0;

    // Transition effect state
    bool    _effectActive = false;
    Effect  _effect       = Effect::NONE;
    uint8_t _effectTimer  = 0;
    uint8_t _effectDuration = 0;
    Scene*  _pendingScene = nullptr;
    Action  _pendingAction = PUSH;

    void _startEffect(Effect effect, Action action, Scene* s) {
        _effectActive = true;
        _effect = effect;
        _effectDuration = (effect == Effect::FADE) ? 12 : 8; // Total frames
        _effectTimer = _effectDuration;
        _pendingAction = action;
        _pendingScene = s;
    }
};