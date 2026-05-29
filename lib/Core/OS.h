#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#include "Config.h"
#include "InputManager.h"
#include "Sound.h"
#include "Battery.h"
#include "SaveManager.h"
#include "Console.h"
#include "GameBase.h"
#include "SystemMenu.h"



class OS {
public:
    OS();
    
    // Call once in setup()
    void begin();
    
    // The infinite game loop. Call in loop().
    void run();

private:
#ifdef SIMULATOR
    U8G2_BITMAP _disp;
#else
    U8G2_SH1106_128X64_NONAME_F_HW_I2C _disp;
#endif
    InputManager _input;
    Sound        _sound;
    Battery      _batt;
    SaveManager  _save;     
    Console      _console;

    static constexpr uint8_t MAX_GAMES = 12;
    GameRecord _games[MAX_GAMES] = {};
    uint8_t   _gameCount        = 0;

    // The Menu is now just a class instance
    SystemMenu _sysMenu;
};