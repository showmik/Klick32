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

    // ── Transitions (safe to call from within Scene::update) ──────────────────

    // Hard-cut: exit all current scenes, enter s.
    void replace(Scene* s, Console& ctx) {
        while (_depth > 0) _stack[--_depth]->onExit(ctx);
        if (s) { _stack[_depth++] = s; s->setManager(this); s->onEnter(ctx); }
    }

    // Overlay: pause the current top, push s on top.
    // Silently ignored if the stack is full.
    void push(Scene* s, Console& ctx) {
        if (!s || _depth >= MAX_DEPTH) return;
        _stack[_depth++] = s;
        s->setManager(this);
        s->onEnter(ctx);
    }

    // Close overlay: exit top, resume the scene beneath.
    // Silently ignored if the stack is already empty.
    void pop(Console& ctx) {
        if (_depth == 0) return;
        _stack[--_depth]->onExit(ctx);
    }

    // Exit everything — game's isRunning() should check empty().
    void clear(Console& ctx) {
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
        if (_depth > 0) _stack[_depth - 1]->update(ctx, *this, dt);
    }

    void draw(Console& ctx) {
        if (_depth > 0) _stack[_depth - 1]->draw(ctx);
    }

    void drawUnder(Console& ctx) {
        if (_depth > 1) _stack[_depth - 2]->draw(ctx);
    }

    bool needsRedraw() const {
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
};