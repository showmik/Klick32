#include "OS.h"

// Initialize the menu, passing it pointers to the game registry and battery
OS::OS()
    : _disp(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA)
    , _console(_disp, _input, _sound, _save)
    , _sysMenu(_games, &_gameCount, &_batt) 
{}

void OS::begin() {
    Serial.begin(115200);
    Wire.begin(PIN_SDA, PIN_SCL);
    _disp.begin();
    _input.begin();
    _sound.begin();
    _batt.begin();
    randomSeed(esp_random());
    
    // Wake up from sleep when MENU1 goes LOW
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_MENU1, 0); 
}

void OS::registerGame(GameBase* game) {
    if (_gameCount < MAX_GAMES && game != nullptr)
        _games[_gameCount++] = game;
}

void OS::run() {
    // The OS starts by running the menu
    GameBase* activeGame = &_sysMenu;
    activeGame->onEnter(_console);

    while (true) {
        uint32_t t0 = millis();
        _input.update();

        // 1. UPDATE LOGIC
        activeGame->update(_console);

        // 2. CONDITIONAL DRAWING
        if (activeGame->needsRedraw()) {
            _disp.clearBuffer();
            activeGame->draw(_console);
            _disp.sendBuffer();
        }

        // 3. STATE TRANSITIONS
        if (!activeGame->isRunning()) {
            
            if (activeGame == &_sysMenu) {
                // Menu wants to exit -> This means a game was selected!
                activeGame = _sysMenu.getLaunchedGame();
                _save.begin(activeGame->getName());  // Open NVS namespace
                activeGame->onEnter(_console);
            } else {
                // Game wants to exit -> Return to the menu!
                activeGame->onExit(_console);
                _save.end();                         // Close NVS namespace
                activeGame = &_sysMenu;
                activeGame->onEnter(_console);
                SFX::menuBack(_sound);
            }
        }

        // 4. FRAME TIMING
        uint32_t elapsed = millis() - t0;
        if (elapsed < FRAME_MS) delay(FRAME_MS - elapsed);
    }
}