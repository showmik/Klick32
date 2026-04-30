#pragma once
#include "GameBase.h"
#include "Battery.h"
#include "Config.h"

class SystemMenu : public GameBase {
public:
    // Takes references to the OS's game list and hardware
    SystemMenu(GameBase** games, const uint8_t* gameCount, Battery* batt);

    void onEnter(Console& ctx) override;
    void onExit(Console& ctx)  override;
    void update(Console& ctx)  override;
    void draw(Console& ctx)    override;
    
    bool           isRunning()   const override;
    const char*    getName()     const override;
    bool           needsRedraw() const override;

    // The OS calls this to find out which game the user selected
    GameBase* getLaunchedGame() const;

private:
    GameBase**     _games;
    const uint8_t* _gameCount;
    Battery*       _batt;

    bool           _running = true;
    bool           _dirty   = true;
    uint8_t        _selected = 0;
    GameBase*      _launchedGame = nullptr;

    uint8_t        _battPct   = 0;
    uint32_t       _battTimer = 0;

    static constexpr uint32_t IDLE_SLEEP_MS = 60000;
    uint32_t       _lastInputTime = 0;

    void _enterDeepSleep(Console& ctx);
    void _drawHeader(Console& ctx);
    void _drawGameCard(Console& ctx, uint8_t idx);
    void _drawFooter(Console& ctx);
};