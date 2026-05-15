#include "OS.h"
#include "GameRegistry.h"

GameRegistryNode* GameRegistryNode::head = nullptr;

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

    // Auto-register games from static linked list
    GameRegistryNode* curr = GameRegistryNode::head;
    while (curr && _gameCount < MAX_GAMES) {
        GameBase* temp = curr->factory();
        _games[_gameCount].factory = curr->factory;
        _games[_gameCount].name = temp->getName();
        _games[_gameCount].icon = temp->getIcon();
        _games[_gameCount].cover = temp->getCoverArt();
        delete temp;
        _gameCount++;
        curr = curr->next;
    }
}

void OS::registerGame(GameBase* game) {
    // Deprecated for dynamically instantiated games.
    // Left empty to prevent compiling errors with old main.cpp if not removed.
}

void OS::run() {
    // The OS starts by running the menu
    GameBase* activeGame = &_sysMenu;
    activeGame->onEnter(_console);
    uint32_t lastTime = millis();

    while (true) {
        uint32_t now = millis();
        float dt = (now - lastTime) / 1000.0f;
        if (dt <= 0.001f) dt = 0.001f;
        lastTime = now;

        _input.update();

        // 1. UPDATE LOGIC
        activeGame->update(_console, dt);

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
                GameRecord* rec = _sysMenu.getLaunchedGameRecord();
                if (rec && rec->factory) {
                    activeGame = rec->factory();
                    _save.begin(rec->name);  // Open NVS namespace
                    activeGame->onEnter(_console);
                }
            } else {
                // Game wants to exit -> Return to the menu!
                activeGame->onExit(_console);
                _save.end();                         // Close NVS namespace
                delete activeGame;                   // DYNAMIC DELETION
                activeGame = &_sysMenu;
                activeGame->onEnter(_console);
                SFX::menuBack(_sound);
            }
        }

        // 4. FRAME TIMING
        uint32_t elapsed = millis() - now;
        if (elapsed < FRAME_MS) {
            // Yield the core to FreeRTOS background tasks instead of spin-blocking
            vTaskDelay(pdMS_TO_TICKS(FRAME_MS - elapsed));
        } else {
            // If we dropped a frame, just yield for 1 tick to feed the watchdog
            vTaskDelay(1); 
        }
    }
}