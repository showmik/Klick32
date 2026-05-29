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

    int savedCpuSpeed = _save.getInt("cpu_speed", 240);
    if (savedCpuSpeed != 240 && savedCpuSpeed != 160 && savedCpuSpeed != 80) {
        savedCpuSpeed = 240;
    }
#ifndef SIMULATOR
    setCpuFrequencyMhz(savedCpuSpeed);
#endif

    bool savedHud = _save.getBool("hud_visible", false);
    if (savedHud != Diagnostics::isVisible()) {
        Diagnostics::toggle();
    }

    // ── Check for WiFi Dashboard Boot (Hold MENU1 + UP) ──
    _input.update();
    if (_input.held(Btn::MENU1) && _input.held(Btn::UP)) {
#ifndef SIMULATOR
        extern void startWifiDashboard(U8G2& disp, InputManager& input);
        startWifiDashboard(_disp, _input);
#endif
    }

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
    uint32_t lastGlobalInputTime = millis();

    static int osCpuSpeed = savedCpuSpeed;

    while (true) {
#ifdef SIMULATOR
        extern bool g_quit;
        if (g_quit) return;
#endif
        uint32_t now = millis();
        float dt = (now - lastTime) / 1000.0f;
        if (dt <= 0.001f) dt = 0.001f;
        lastTime = now;

        static bool osOverlayActive = false;
        static int  osOverlayCursor = 0;
        static float osOverlayAnim = 0.0f;

        _input.update();

        // Update overlay animation
        if (osOverlayActive) {
            if (osOverlayAnim < 1.0f) {
                osOverlayAnim += dt * 8.0f;
                if (osOverlayAnim > 1.0f) osOverlayAnim = 1.0f;
            }
        } else {
            if (osOverlayAnim > 0.0f) {
                osOverlayAnim -= dt * 8.0f;
                if (osOverlayAnim < 0.0f) osOverlayAnim = 0.0f;
            }
        }
        
        // ── Global Idle Sleep Check ──
        bool anyInput = false;
        for (uint8_t i = 0; i < (uint8_t)Btn::COUNT; i++) {
            if (_input.held((Btn)i)) { anyInput = true; break; }
        }
        
        if (anyInput) {
            lastGlobalInputTime = now;
        } else if (now - lastGlobalInputTime >= 60000UL) { // 60 seconds
            _disp.clearBuffer();
            _disp.setDrawColor(0);
            _disp.drawBox(0, 0, 128, 64);
            _disp.setDrawColor(1);
            _disp.setFont(u8g2_font_6x10_tf);
            int w = _disp.getStrWidth("Sleeping...");
            _disp.drawStr((128 - w) / 2, 34, "Sleeping...");
            _disp.sendBuffer();
            delay(1000);
            
            _save.commit(); // Force write dirty cache to flash
            _sound.stop();
#ifndef SIMULATOR
            _disp.setPowerSave(1);
            esp_deep_sleep_start();
#endif
        }

        if (_console.pressed(Btn::MENU1) && _console.justPressed(Btn::MENU2)) {
            Diagnostics::toggle();
            
            // Switch to OS namespace temporarily to save HUD state
            const char* currentGameName = activeGame->getName();
            _save.end();
            _save.begin("__os");
            _save.putBool("hud_visible", Diagnostics::isVisible());
            _save.end();
            _save.begin(currentGameName);
            
            osOverlayActive = false;
        } else if (activeGame != &_sysMenu && _console.justPressed(Btn::MENU2)) {
            osOverlayActive = !osOverlayActive;
            osOverlayCursor = 0;
            if (osOverlayActive) SFX::menuEnter(_sound);
            else SFX::menuBack(_sound);
        }

        // 1. UPDATE LOGIC
        if (osOverlayActive) {
            if (_console.justPressed(Btn::UP)) {
                osOverlayCursor--;
                if (osOverlayCursor < 0) osOverlayCursor = 4;
                SFX::menuNav(_sound);
            }
            if (_console.justPressed(Btn::DOWN)) {
                osOverlayCursor++;
                if (osOverlayCursor > 4) osOverlayCursor = 0;
                SFX::menuNav(_sound);
            }
            if (_console.justPressed(Btn::A)) {
                if (osOverlayCursor == 0) {
                    SFX::menuEnter(_sound);
                    osOverlayActive = false; // Resume
                } else if (osOverlayCursor == 1) {
                    _sound.toggleMute(); // Toggle Mute
                    
                    // Switch to OS namespace temporarily to save Mute
                    const char* currentGameName = activeGame->getName();
                    _save.end();
                    _save.begin("__os");
                    _save.putBool("mute", _sound.isMuted());
                    _save.end();
                    _save.begin(currentGameName);
                    
                    SFX::menuEnter(_sound);
                } else if (osOverlayCursor == 2) {
                    // CPU Speed Cycle: 240 -> 160 -> 80
                    if (osCpuSpeed == 240) osCpuSpeed = 160;
                    else if (osCpuSpeed == 160) osCpuSpeed = 80;
                    else osCpuSpeed = 240;
                    
                    _console.setCPUSpeed(osCpuSpeed);
                    
                    // Switch to OS namespace temporarily to save CPU Speed
                    const char* currentGameName = activeGame->getName();
                    _save.end();
                    _save.begin("__os");
                    _save.putInt("cpu_speed", osCpuSpeed);
                    _save.end();
                    _save.begin(currentGameName);
                    
                    SFX::menuEnter(_sound);
                } else if (osOverlayCursor == 3) {
                    Diagnostics::toggle();
                    
                    // Switch to OS namespace temporarily to save HUD state
                    const char* currentGameName = activeGame->getName();
                    _save.end();
                    _save.begin("__os");
                    _save.putBool("hud_visible", Diagnostics::isVisible());
                    _save.end();
                    _save.begin(currentGameName);
                    
                    SFX::menuEnter(_sound);
                } else if (osOverlayCursor == 4) {
                    // Quit Game
                    osOverlayActive = false;
                    
                    activeGame->onExit(_console);
                    _save.end(); // triggers commit
#ifndef SIMULATOR
                    setCpuFrequencyMhz(240);
#endif
                    activeGame = &_sysMenu;
                    _save.begin("__os");
                    activeGame->onEnter(_console);
                    SFX::menuBack(_sound);
                    continue; // Skip the rest of this frame
                }
            }
            if (_console.justPressed(Btn::B)) {
                osOverlayActive = false;
                SFX::menuBack(_sound);
            }
        } else {
            Diagnostics::markUpdateStart();
            activeGame->update(_console, dt);
            Diagnostics::markUpdateEnd();
        }

        // 2. CONDITIONAL DRAWING
        if (activeGame->needsRedraw() || Diagnostics::isVisible() || osOverlayActive || osOverlayAnim > 0.0f) {
            _disp.clearBuffer();
            activeGame->draw(_console);
            
            if (osOverlayAnim > 0.0f) {
                // 1. Full Screen Dither Dimming (XOR checkerboard)
                _console.beginScreenSpace();
                _console.setDrawColor(Console::COLOR_XOR);
                _console.drawDitherBox(0, 0, 128, 64, 2);
                _console.endScreenSpace();
                
                // 2. Centered Modal Dialog
                int h = (int)(56 * osOverlayAnim);
                int y = 4 + (56 - h) / 2;
                
                _console.beginScreenSpace();
                
                // Solid black background
                _console.setDrawColor(Console::COLOR_BLACK);
                _console.drawRBox(10, y, 108, h, 4);
                
                // White rounded border
                _console.setDrawColor(Console::COLOR_WHITE);
                _console.drawRFrame(10, y, 108, h, 4);
                
                // Content only if fully or mostly open
                if (osOverlayAnim >= 0.9f) {
                    _console.drawHLine(16, y + 11, 96);
                    
                    _console.setFont(u8g2_font_5x7_tf);
                    _console.drawStrCentered(y + 9, "QUICK SETTINGS");
                    
                    const char* items[5];
                    items[0] = "Resume Game";
                    
                    char soundBuf[16];
                    snprintf(soundBuf, sizeof(soundBuf), "Audio: %s", _sound.isMuted() ? "OFF" : "ON");
                    items[1] = soundBuf;
                    
                    char cpuBuf[20];
                    snprintf(cpuBuf, sizeof(cpuBuf), "CPU: %s", (osCpuSpeed == 240) ? "Max 240M" : (osCpuSpeed == 160) ? "Bal 160M" : "Eco 80M");
                    items[2] = cpuBuf;
                    
                    char hudBuf[20];
                    snprintf(hudBuf, sizeof(hudBuf), "Debug HUD: %s", Diagnostics::isVisible() ? "ON" : "OFF");
                    items[3] = hudBuf;
                    
                    items[4] = "Quit to Menu";
                    
                    for (int i = 0; i < 5; i++) {
                        int itemY = y + 20 + (i * 8);
                        if (i == osOverlayCursor) {
                            _console.drawStr(18, itemY, ">");
                        }
                        _console.drawStr(25, itemY, items[i]);
                    }
                    
                    // Draw selection highlight pill
                    int selectY = y + 20 + (osOverlayCursor * 8);
                    _console.setDrawColor(Console::COLOR_XOR);
                    _console.drawRBox(16, selectY - 7, 96, 9, 2);
                }
                
                _console.endScreenSpace();
            }

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
#ifndef SIMULATOR
                    setCpuFrequencyMhz(osCpuSpeed); // Apply configured CPU Speed
#endif
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