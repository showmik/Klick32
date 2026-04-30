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

    // These should only appear ONCE in the class
    void _enterDeepSleep(Console& ctx);
    void _drawHeader(Console& ctx);
    void _drawGameCard(Console& ctx, uint8_t idx);
    void _drawFooter(Console& ctx);

    // ── Layout Design System ──────────────────────────────────────────────────
    struct Layout {
        // Header
        static constexpr int HDR_TEXT_Y     = 7;
        static constexpr int HDR_LINE_Y     = 9;
        static constexpr int BATT_BOX_X     = 98;
        static constexpr int BATT_BOX_Y     = 1;
        static constexpr int BATT_BOX_W     = 28;
        static constexpr int BATT_BOX_H     = 7;
        static constexpr int BATT_TXT_X     = 74;

        // Game Card
        static constexpr int CARD_ICON_X    = 56;
        static constexpr int CARD_ICON_Y    = 12;
        static constexpr int CARD_ICON_SIZE = 16;
        static constexpr int CARD_NAME_Y    = 41;
        static constexpr int CARD_PAGE_Y    = 50;
        static constexpr int ARROW_L_X      = 0;
        static constexpr int ARROW_R_X      = 122;
        static constexpr int ARROW_Y        = 32;

        // Footer
        static constexpr int FTR_LINE_Y     = 53;
        static constexpr int FTR_TEXT_X     = 4;
        static constexpr int FTR_TEXT_Y     = 61;
    };
};