#include "OS.h"
#include "GameRegistry.h"
#include "Diagnostics.h"

GameRegistryNode* GameRegistryNode::head = nullptr;

// Initialize the menu, passing it pointers to the game registry and battery
#ifdef SIMULATOR
OS::OS()
    : _disp(128, 64, &u8g2_cb_r0)
    , _console(_disp, _input, _sound, _save)
    , _sysMenu(_games, &_gameCount, &_batt) 
{}
#else
OS::OS()
    : _disp(U8G2_R0, U8X8_PIN_NONE, PIN_SCL, PIN_SDA)
    , _console(_disp, _input, _sound, _save)
    , _sysMenu(_games, &_gameCount, &_batt) 
{}
#endif

#ifdef SIMULATOR
uint8_t* g_simDisplayBuffer = nullptr;
#endif

void OS::begin() {
#ifndef SIMULATOR
    setCpuFrequencyMhz(240); // Max power by default
#endif
    Serial.begin(115200);
    Wire.begin(PIN_SDA, PIN_SCL);
    _disp.setBusClock(1000000); // Massive hardware I2C speed boost (1MHz)
    _disp.begin();
    _input.begin();
    _sound.begin();
    _batt.begin();
    Diagnostics::begin();
    randomSeed(esp_random());
    
#ifdef SIMULATOR
    static uint8_t sim_buf[128*64/8];
    _disp.getU8g2()->tile_buf_ptr = sim_buf;
    g_simDisplayBuffer = sim_buf;
#else
    // Wake up from sleep when MENU1 goes LOW
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_MENU1, 0); 
#endif

    // Auto-register games from static linked list
    GameRegistryNode* curr = GameRegistryNode::head;
    while (curr && _gameCount < MAX_GAMES) {
        GameBase* temp = curr->factory();
        _games[_gameCount].factory = curr->factory;
        _games[_gameCount].name = temp->getName();
        _games[_gameCount].icon = temp->getIcon();
        _games[_gameCount].cover = temp->getCoverArt();
        // Do NOT delete temp; it is a static instance
        _gameCount++;
        curr = curr->next;
    }
}

void OS::registerGame(GameBase* game) {
    // Deprecated for dynamically instantiated games.
    // Left empty to prevent compiling errors with old main.cpp if not removed.
}

void OS::run() {
    // ── Boot Settings & Splash ──
    _save.begin("__os");
    bool isMuted = _save.getBool("mute", false);
    _sound.setMuted(isMuted);

    // Minimalist Boot Splash
    uint32_t splashStart = millis();
    while (millis() - splashStart < 1500) {
        _disp.clearBuffer();
        
        // Animated expanding circle
        int radius = (millis() - splashStart) / 30;
        if (radius > 120) radius = 120;
        _disp.setDrawColor(Console::COLOR_WHITE);
        _disp.drawDisc(64, 32, radius);
        
        // Inverted text
        _disp.setDrawColor(Console::COLOR_BLACK);
        _disp.setFont(u8g2_font_ncenB14_tr);
        _disp.drawStr(32, 40, "Klick32");
        
        _disp.sendBuffer();
#ifdef SIMULATOR
        extern void sim_commit_buffer();
        sim_commit_buffer();
#endif
        delay(16);
    }
    
    // Restore default draw state after splash
    _disp.setDrawColor(Console::COLOR_WHITE);
    _disp.setFont(u8g2_font_5x7_tf);
    
    // The OS starts by running the menu (with __os namespace open)
    GameBase* activeGame = &_sysMenu;
    activeGame->onEnter(_console);
    uint32_t lastTime = millis();

    while (true) {
#ifdef SIMULATOR
        extern bool g_quit;
        if (g_quit) return;
#endif
        uint32_t now = millis();
        float dt = (now - lastTime) / 1000.0f;
        if (dt <= 0.001f) dt = 0.001f;
        lastTime = now;

        _input.update();

        if (_console.justPressed(Btn::MENU2)) {
            Diagnostics::toggle();
        }

        // 1. UPDATE LOGIC
        Diagnostics::markUpdateStart();
        activeGame->update(_console, dt);
        Diagnostics::markUpdateEnd();

        // 2. CONDITIONAL DRAWING
        if (activeGame->needsRedraw() || Diagnostics::isVisible()) {
            _disp.clearBuffer();
            activeGame->draw(_console);
            Diagnostics::draw(_console);
            _disp.sendBuffer();
#ifdef SIMULATOR
            extern void sim_commit_buffer();
            sim_commit_buffer();
#endif
        }

        // 3. STATE TRANSITIONS
        if (!activeGame->isRunning()) {
            
            if (activeGame == &_sysMenu) {
                // Menu wants to exit -> This means a game was selected!
                GameRecord* rec = _sysMenu.getLaunchedGameRecord();
                if (rec && rec->factory) {
                    _save.end(); // Close __os
                    activeGame = rec->factory();
                    _save.begin(rec->name);  // Open game namespace
                    activeGame->onEnter(_console);
                }
            } else {
                // Game wants to exit -> Return to the menu!
                activeGame->onExit(_console);
                _save.end();                         // Close game namespace
#ifndef SIMULATOR
                setCpuFrequencyMhz(240);             // Restore max performance for OS Menu
#endif
                // Do NOT delete activeGame; it is a static instance
                activeGame = &_sysMenu;
                _save.begin("__os");                 // Reopen OS namespace
                activeGame->onEnter(_console);
                SFX::menuBack(_sound);
            }
        }
        
        // 4. AUTO-SAVE WEAR LEVELING CACHE (Every 60 seconds)
        static uint32_t lastSaveTime = 0;
        if (now - lastSaveTime > 60000) {
            _save.commit();
            lastSaveTime = now;
        }

        // 5. FRAME TIMING
        Diagnostics::tick();
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