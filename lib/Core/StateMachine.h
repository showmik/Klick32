#pragma once
#include "Console.h"

// ─── StateMachine ────────────────────────────────────────────────────────────
// A lightweight FSM (Finite State Machine) for simpler games.
// For games that don't need a full SceneManager (like Pong, Snake, Dino),
// this manages game states (Title, Playing, GameOver) effortlessly.
// ─────────────────────────────────────────────────────────────────────────────

template<typename TState>
struct StateCallbacks {
    void (*update)(Console&, float) = nullptr;
    void (*draw)(Console&) = nullptr;
    void (*onEnter)(Console&) = nullptr;
    void (*onExit)(Console&) = nullptr;
};

template<typename TState, int MAX_STATES = 8>
class StateMachine {
public:
    StateMachine() {
        for (int i = 0; i < MAX_STATES; i++) {
            _states[i].id = (TState)-1;
        }
    }

    void addState(TState id, StateCallbacks<TState> callbacks) {
        for (int i = 0; i < MAX_STATES; i++) {
            if (_states[i].id == (TState)-1 || _states[i].id == id) {
                _states[i].id = id;
                _states[i].callbacks = callbacks;
                return;
            }
        }
    }

    void transition(Console& ctx, TState nextState) {
        if (_currentState != (TState)-1) {
            int idx = _findState(_currentState);
            if (idx >= 0 && _states[idx].callbacks.onExit) {
                _states[idx].callbacks.onExit(ctx);
            }
        }
        
        _currentState = nextState;
        
        int idx = _findState(_currentState);
        if (idx >= 0 && _states[idx].callbacks.onEnter) {
            _states[idx].callbacks.onEnter(ctx);
        }
    }

    void update(Console& ctx, float dt) {
        if (_currentState == (TState)-1) return;
        int idx = _findState(_currentState);
        if (idx >= 0 && _states[idx].callbacks.update) {
            _states[idx].callbacks.update(ctx, dt);
        }
    }

    void draw(Console& ctx) {
        if (_currentState == (TState)-1) return;
        int idx = _findState(_currentState);
        if (idx >= 0 && _states[idx].callbacks.draw) {
            _states[idx].callbacks.draw(ctx);
        }
    }

    TState current() const {
        return _currentState;
    }

private:
    struct StateData {
        TState id;
        StateCallbacks<TState> callbacks;
    };
    
    StateData _states[MAX_STATES];
    TState _currentState = (TState)-1;

    int _findState(TState id) const {
        for (int i = 0; i < MAX_STATES; i++) {
            if (_states[i].id == id) return i;
        }
        return -1;
    }
};
