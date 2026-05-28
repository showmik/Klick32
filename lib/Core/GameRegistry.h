#pragma once
#include "GameBase.h"

typedef GameBase* (*GameFactory)();

struct GameRecord {
    GameFactory factory;
    const char* name;
    const uint8_t* icon;
    const uint8_t* cover;
};

struct GameRegistryNode {
    GameFactory factory;
    GameRegistryNode* next;
    static GameRegistryNode* head;
};

#define REGISTER_GAME(ClassName) \
    static GameBase* _factory_##ClassName() { static ClassName instance; return &instance; } \
    static GameRegistryNode _node_##ClassName = { _factory_##ClassName, nullptr }; \
    struct _Register_##ClassName { \
        _Register_##ClassName() { \
            _node_##ClassName.next = GameRegistryNode::head; \
            GameRegistryNode::head = &_node_##ClassName; \
        } \
    } _reg_##ClassName;
