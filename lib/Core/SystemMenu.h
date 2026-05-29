#pragma once
#include "GameBase.h"
#include "GameRegistry.h"
#include "Battery.h"
#include "Config.h"

class SystemMenu : public GameBase {
public:
    // Takes references to the OS's game list and hardware
    SystemMenu(GameRecord* games, const uint8_t* gameCount, Battery* batt);

    void onEnter(Console& ctx) override;
    void onExit(Console& ctx)  override;
    void update(Console& ctx, float dt)  override;
    void draw(Console& ctx)    override;
    
    bool           isRunning()   const override;
    const char*    getName()     const override;
    bool           needsRedraw() const override;

    // The OS calls this to find out which game the user selected
    GameRecord* getLaunchedGameRecord() const;

private:
    GameRecord*    _games;
    const uint8_t* _gameCount;
    Battery*       _batt;

    bool           _running = true;
    bool           _dirty   = true;
    uint8_t        _selected = 0;
    GameRecord*    _launchedGame = nullptr;

    uint8_t        _battPct   = 0;
    uint32_t       _battTimer = 0;

    int            _slideOffset  = 0;
    uint8_t        _prevSelected = 0;

    static constexpr uint32_t IDLE_SLEEP_MS = 60000;
    uint32_t       _lastInputTime = 0;
    uint8_t        _animTick = 0;
    uint8_t        _arrowTick = 0;
    bool           _inAboutPage = false;

    // These should only appear ONCE in the class
    void _enterDeepSleep(Console& ctx);
    void _drawHeader(Console& ctx);
    void _drawGameCard(Console& ctx, uint8_t idx, int offsetX); // Now takes an offset
    void _drawPagination(Console& ctx, uint8_t idx);            // Extracted static UI
    void _drawFooter(Console& ctx);
    void _drawAboutPage(Console& ctx);

    struct Layout {
        // Header
        static constexpr int HDR_TEXT_Y     = 7;
        static constexpr int HDR_LINE_Y     = 9;
        static constexpr int BATT_BOX_X     = 98;
        static constexpr int BATT_BOX_Y     = 1;
        static constexpr int BATT_BOX_W     = 28;
        static constexpr int BATT_BOX_H     = 7;
        static constexpr int BATT_TXT_X     = 74;

        // Game Card (FULL SCREEN, NO MARGIN)
        static constexpr int CARD_FRAME_X   = 0;     // Touch left edge
        static constexpr int CARD_FRAME_Y   = 10;    // Touch header line
        static constexpr int CARD_FRAME_W   = 128;   // Full screen width
        static constexpr int CARD_FRAME_H   = 45;    // Extends down to footer line
        
        static constexpr int CARD_ICON_X    = 56;
        static constexpr int CARD_ICON_Y    = 20;
        static constexpr int CARD_ICON_SIZE = 16;
        static constexpr int CARD_NAME_Y    = 42;
        
        static constexpr int ARROW_L_X      = 2;     // Floating left arrow
        static constexpr int ARROW_R_X      = 120;   // Floating right arrow
        static constexpr int ARROW_Y        = 34;
        
        // Cover Art exactly matches the frame
        static constexpr int CARD_COVER_W   = 128;
        static constexpr int CARD_COVER_H   = 45;
        static constexpr int CARD_COVER_BPR = 16;    // ceil(128 / 8)
        
        // Footer
        static constexpr int FTR_LINE_Y     = 55;
        static constexpr int FTR_TEXT_X     = 4;
        static constexpr int FTR_TEXT_Y     = 63;
    };
};