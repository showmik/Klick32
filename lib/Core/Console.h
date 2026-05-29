#pragma once
#include <U8g2lib.h>
#include "InputManager.h"
#include "Sound.h"
#include "SaveManager.h"
#include "Camera.h"
#include <stdarg.h>

#ifdef __GNUC__
#define KLICK32_PRINTF_LIKE(fmt_arg, var_arg) __attribute__((format(printf, fmt_arg, var_arg)))
#else
#define KLICK32_PRINTF_LIKE(fmt_arg, var_arg)
#endif

// ─── Console ──────────────────────────────────────────────────────────────────
// The single context object passed to every game's update() and draw() calls.
//
// Purpose: decouple games from hardware entirely.
//   • Games never #include U8g2lib.h, InputManager.h, Sound.h, or
//     SaveManager.h directly.
//   • Swapping any driver only requires editing Console — zero game files change.
//
// Construction: only OS may construct a Console (private ctor + friend).
// Games receive it by reference each frame and never store it.
//
// Escape hatch: call ctx.gfx() to get the raw U8G2& for anything not
// covered by the drawing API below.  Use sparingly.
//
// Screen origin (0, 0) is top-left.
// For text, y is the glyph BASELINE, not the top of the character.
// ─────────────────────────────────────────────────────────────────────────────

class OS;



class Console {
public:

    // ── Screen constants ──────────────────────────────────────────────────────
    static constexpr int W = 128;
    static constexpr int H = 64;

    // ══════════════════════════════════════════════════════════════════════════
    // HARDWARE POWER SCALING
    // ══════════════════════════════════════════════════════════════════════════

    // Dynamically scale the ESP32 CPU Frequency to save battery.
    // Allowed values: 240 (Max Performance), 160, 80 (Max Battery)
    void setCPUSpeed(int mhz) {
#ifndef SIMULATOR
        setCpuFrequencyMhz(mhz);
#endif
    }

    // ══════════════════════════════════════════════════════════════════════════
    // INPUT
    // ══════════════════════════════════════════════════════════════════════════

    // True every frame the button is held (debounced).
    // Use for continuous actions: walking, charging a shot.
    bool pressed(Btn b) const {
        return _input.held(b);
    }

    // True on the ONE frame the button transitions unpressed → pressed.
    // Use for one-shot actions: jump, shoot, menu confirm.
    bool justPressed(Btn b) const {
        return _input.justPressed(b);
    }

    // True on the ONE frame the button transitions pressed → unpressed.
    bool justReleased(Btn b) const {
        return _input.justReleased(b);
    }

    // True on justPressed AND periodically while held.
    // Use for menu navigation: scroll, cursor move.
    bool repeat(Btn b) const {
        return _input.repeat(b);
    }

    // Consecutive frames the button has been held. Resets to 0 on release.
    // Useful for charge mechanics and long-press detection.
    //   if (ctx.holdFrames(Btn::A) == 60) { /* held for ~2 s */ }
    uint16_t holdFrames(Btn b) const {
        return _input.holdFrames(b);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // SOUND
    // ══════════════════════════════════════════════════════════════════════════

    // Play a raw tone at freqHz for durationMs milliseconds (non-blocking).
    void beep(uint16_t freqHz, uint32_t durationMs = 50) {
        _sound.beep(freqHz, durationMs);
    }

    void stopSound()      { _sound.stop();        }
    void setMuted(bool m) { _sound.setMuted(m);   }
    void toggleMute()     { _sound.toggleMute();  }
    bool isMuted()  const { return _sound.isMuted(); }

    // ── Predefined sound effects ───────────────────────────────────────────────
    // Prefer these over raw beep() so effects stay consistent across games.
    void sfxMenuNav()   { SFX::menuNav(_sound); }
    void sfxMenuEnter() { SFX::menuEnter(_sound); }
    void sfxMenuBack()  { SFX::menuBack(_sound); }
    void sfxJump()      { SFX::jump(_sound); }
    void sfxDeath()     { SFX::death(_sound); }
    void sfxPoint()     { SFX::point(_sound); }
    
    // Play a background music track (array of ToneSteps) asynchronously on Core 0!
    void playTrack(const ToneStep* sequence) { _sound.playTrack(sequence); }

    // ══════════════════════════════════════════════════════════════════════════
    // SAVE / LOAD   (NVS — persists across power cycles)
    // ══════════════════════════════════════════════════════════════════════════
    //
    // The OS automatically opens and closes the correct NVS namespace when a
    // game is launched or exits.  Games only call these methods; they never
    // manage namespaces themselves.
    //
    // Key rules:
    //   • Max 15 characters per key (NVS limit).
    //   • Use short, stable names: "hi", "level", "coins", "cfg_sfx".
    //   • Keys are per-game — two games can both use "hi" with no conflict.
    //
    // Typical usage in a game:
    //
    //   void onEnter() override {
    //       _hiScore = ctx.loadUInt("hi");      // load once on launch
    //   }
    //
    //   void onExit() override {
    //       ctx.saveUInt("hi", _hiScore);       // flush on exit
    //   }
    //
    //   // — or use the shortcut —
    //   void onExit() override {
    //       ctx.updateHiScore(_hiScore);        // only writes if new record
    //   }

    // ── Write ─────────────────────────────────────────────────────────────────
    void saveUInt (const char* key, uint32_t v) { _save.putUInt (key, v); }
    void saveInt  (const char* key, int32_t  v) { _save.putInt  (key, v); }
    void saveFloat(const char* key, float    v) { _save.putFloat(key, v); }
    void saveBool (const char* key, bool     v) { _save.putBool (key, v); }
    void saveByte (const char* key, uint8_t  v) { _save.putByte (key, v); }
    void saveBytes(const char* key, const void* v, size_t len) { _save.putBytes(key, v, len); }
    void saveStr  (const char* key, const char* v)              { _save.putStr(key, v); }

    // ── Read ──────────────────────────────────────────────────────────────────
    uint32_t loadUInt (const char* key, uint32_t def = 0)    { return _save.getUInt (key, def); }
    int32_t  loadInt  (const char* key, int32_t  def = 0)    { return _save.getInt  (key, def); }
    float    loadFloat(const char* key, float    def = 0.0f) { return _save.getFloat(key, def); }
    bool     loadBool (const char* key, bool     def = false) { return _save.getBool (key, def); }
    uint8_t  loadByte (const char* key, uint8_t  def = 0)    { return _save.getByte (key, def); }
    size_t   loadBytes(const char* key, void* buf, size_t maxLen) { return _save.getBytes(key, buf, maxLen); }
    size_t   loadStr  (const char* key, char* buf, size_t bufSize, const char* def = "") { return _save.getStr(key, buf, bufSize, def); }

    // True if the key has been written at least once.
    bool hasSave(const char* key) { return _save.hasKey(key); }

    // ── Hi-score shortcut ─────────────────────────────────────────────────────
    // updateHiScore() only writes to NVS when score beats the stored value.
    // Returns true when a new record is set — useful for triggering a fanfare.
    //
    //   if (ctx.updateHiScore(_score)) ctx.sfxPoint();
    bool     updateHiScore(uint32_t score)  { return _save.updateHiScore(score); }
    void     saveHiScore  (uint32_t score)  { _save.saveHiScore(score); }
    uint32_t loadHiScore  ()                { return _save.loadHiScore(); }

    // ── Erase ─────────────────────────────────────────────────────────────────
    // Wipe ALL saved data for the current game.
    // Typically surfaced as "Clear Data" in a game's settings screen.
    void clearSaveData() { _save.clearAll(); }

    // Remove a single key.
    void removeSave(const char* key) { _save.remove(key); }

    // ══════════════════════════════════════════════════════════════════════════
    // CAMERA
    // ══════════════════════════════════════════════════════════════════════════
    void setCamera(Camera* cam) { _camera = cam; }
    Camera* getCamera() const { return _camera; }

    // Call this before drawing HUD elements (score, lives) that should not scroll.
    void beginScreenSpace() { _screenSpace = true; }

    // Call this after drawing the HUD to resume scrolling for game world elements.
    void endScreenSpace() { _screenSpace = false; }

    // ══════════════════════════════════════════════════════════════════════════
    // DRAWING
    // ══════════════════════════════════════════════════════════════════════════

    static constexpr uint8_t COLOR_BLACK = 0; // clear/transparent
    static constexpr uint8_t COLOR_WHITE = 1; // set/solid
    static constexpr uint8_t COLOR_XOR   = 2; // invert

    // Save/restore draw color across nested drawing operations.
    void pushDrawState() {
        if (_drawStackCount < 4) {
            _drawStack[_drawStackCount].color = _disp.getDrawColor();
            _drawStackCount++;
        }
    }

    void popDrawState() {
        if (_drawStackCount > 0) {
            _drawStackCount--;
            setDrawColor(_drawStack[_drawStackCount].color);
        }
    }

    void setDrawColor(uint8_t color)  { _disp.setDrawColor(color); }
    void setFont(const uint8_t* font) { _disp.setFont(font);       }

    // Pixel width of str in the currently-set font.
    int strWidth(const char* str) { return (int)_disp.getStrWidth(str); }

    // ── Pixels and lines ──────────────────────────────────────────────────────
    void drawPixel(int x, int y)                    { _disp.drawPixel(x + _camX(), y + _camY());           }
    void drawHLine(int x, int y, int w)             { _disp.drawHLine(x + _camX(), y + _camY(), w);        }
    void drawVLine(int x, int y, int h)             { _disp.drawVLine(x + _camX(), y + _camY(), h);        }
    void drawLine (int x1, int y1, int x2, int y2) { _disp.drawLine(x1 + _camX(), y1 + _camY(), x2 + _camX(), y2 + _camY()); }

    // ── Rectangles & Dither ───────────────────────────────────────────────────
    void drawFrame (int x, int y, int w, int h)        { _disp.drawFrame(x + _camX(), y + _camY(), w, h);     }
    void drawBox   (int x, int y, int w, int h)        { _disp.drawBox(x + _camX(), y + _camY(), w, h);       }
    void drawRFrame(int x, int y, int w, int h, int r) { _disp.drawRFrame(x + _camX(), y + _camY(), w, h, r); }
    void drawRBox  (int x, int y, int w, int h, int r) { _disp.drawRBox(x + _camX(), y + _camY(), w, h, r);   }

    // Draws a shaded box using spatial dithering (0=Black, 1=DarkGray 25%, 2=LightGray 50%, 3=Silvery 75%, 4=White)
    void drawDitherBox(int x, int y, int w, int h, uint8_t shade) {
        if (shade == 0) return;
        
        int cx = x + _camX();
        int cy = y + _camY();
        int clipX = max(0, cx);
        int clipY = max(0, cy);
        int clipMaxX = min((int)W, cx + w);
        int clipMaxY = min((int)H, cy + h);
        
        if (clipX >= clipMaxX || clipY >= clipMaxY) return;

        if (shade >= 4) {
            _disp.drawBox(clipX, clipY, clipMaxX - clipX, clipMaxY - clipY);
            return;
        }

        for(int py = clipY; py < clipMaxY; py++) {
            for(int px = clipX; px < clipMaxX; px++) {
                bool draw = false;
                if (shade == 1)      { draw = (px % 2 == 0) && (py % 2 == 0); }           // 25% pattern
                else if (shade == 2) { draw = ((px + py) % 2) == 0; }                     // 50% checkerboard
                else if (shade == 3) { draw = !((px % 2 != 0) && (py % 2 != 0)); }        // 75% pattern
                
                if (draw) _disp.drawPixel(px, py);
            }
        }
    }

    // ── Circles ───────────────────────────────────────────────────────────────
    void drawCircle(int cx, int cy, int r) { _disp.drawCircle(cx + _camX(), cy + _camY(), r); }
    void drawDisc  (int cx, int cy, int r) { _disp.drawDisc(cx + _camX(), cy + _camY(), r);   }

    // ── Text ──────────────────────────────────────────────────────────────────
    // y is the baseline, not the top of the character.
    void drawStr(int x, int y, const char* str) { _disp.drawStr(x + _camX(), y + _camY(), str); }

    // Horizontally centered text.
    void drawStrCentered(int y, const char* str) {
        int w = (int)_disp.getStrWidth(str);
        _disp.drawStr((W - w) / 2 + _camX(), y + _camY(), str);
    }

    // Centered both horizontally and vertically.
    // Vertical centering uses font ascent (approximate — assumes ~7px glyph height).
    void drawStrCenteredBoth(const char* str) {
        int w = (int)_disp.getStrWidth(str);
        int fontH = _disp.getMaxCharHeight();
        _disp.drawStr((W - w) / 2 + _camX(), (H + fontH) / 2 + _camY(), str);
    }

    // Right-aligned text.
    //   ctx.drawStrRight(W - 2, 10, "99");
    void drawStrRight(int x, int y, const char* str) {
        int w = (int)_disp.getStrWidth(str);
        _disp.drawStr(x - w + _camX(), y + _camY(), str);
    }


    // Printf-style formatted text.
    // Eliminates the char buf[32]; snprintf(buf,...); drawStr(x,y,buf) pattern.
    //   ctx.drawPrintf(2, 10, "Score: %d", score);
    void drawPrintf(int x, int y, const char* fmt, ...) KLICK32_PRINTF_LIKE(4, 5) {
        char buf[48];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        _disp.drawStr(x + _camX(), y + _camY(), buf);
    }

    // Printf-style, horizontally centered.
    //   ctx.drawPrintfCentered(32, "Level %d", level);
    void drawPrintfCentered(int y, const char* fmt, ...) KLICK32_PRINTF_LIKE(3, 4) {
        char buf[48];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        int w = (int)_disp.getStrWidth(buf);
        _disp.drawStr((W - w) / 2 + _camX(), y + _camY(), buf);
    }

    // ── Bitmaps & Tilemaps ────────────────────────────────────────────────────
    
    // Flip flags for drawBitmapEx
    static constexpr uint8_t BMP_FLIP_NONE = 0x00;
    static constexpr uint8_t BMP_FLIP_H    = 0x01;
    static constexpr uint8_t BMP_FLIP_V    = 0x02;
    static constexpr uint8_t BMP_FLIP_HV   = 0x03;

    // Fluent builder for drawing bitmaps.
    // Eliminates positional parameter confusion.
    // Usage: ctx.blit(bmp, bytesPerRow, h).at(x, y).flipX().draw();
    class BitmapBuilder {
        Console& _ctx;
        const uint8_t* _bmp;
        int _bytesPerRow;
        int _h;
        int _x = 0;
        int _y = 0;
        uint8_t _flags = BMP_FLIP_NONE;
    public:
        BitmapBuilder(Console& ctx, const uint8_t* bmp, int bytesPerRow, int h)
            : _ctx(ctx), _bmp(bmp), _bytesPerRow(bytesPerRow), _h(h) {}
        
        BitmapBuilder& at(int x, int y) { _x = x; _y = y; return *this; }
        BitmapBuilder& flipX(bool flip = true) { if(flip) _flags |= BMP_FLIP_H; else _flags &= ~BMP_FLIP_H; return *this; }
        BitmapBuilder& flipY(bool flip = true) { if(flip) _flags |= BMP_FLIP_V; else _flags &= ~BMP_FLIP_V; return *this; }
        void draw() { _ctx.drawBitmapEx(_x, _y, _bytesPerRow, _h, _bmp, _flags); }
    };

    BitmapBuilder blit(const uint8_t* bmp, int bytesPerRow, int h) {
        return BitmapBuilder(*this, bmp, bytesPerRow, h);
    }

    // Standard drawBitmap (unmodified U8g2 passthrough)
    // bytesPerRow = ceil(spriteWidthPx / 8).
    // bmp must be in PROGMEM.
    void drawBitmap(int x, int y, int bytesPerRow, int h, const uint8_t* bmp) {
        _disp.drawBitmap(x + _camX(), y + _camY(), bytesPerRow, h, bmp);
    }

    // Extended drawBitmap with flip support and clipping
    void drawBitmapEx(int x, int y, int bytesPerRow, int h, const uint8_t* bmp, uint8_t flags = BMP_FLIP_NONE) {
        int cx = x + _camX();
        int cy = y + _camY();
        int w = bytesPerRow * 8;
        
        // Fast path for no flipping
        if (flags == BMP_FLIP_NONE) {
            _disp.drawBitmap(cx, cy, bytesPerRow, h, bmp);
            return;
        }

        // Slow path for flipped (software pixel plotting)
        // Optimization: check if bounding box is entirely off-screen
        if (cx >= W || cx + w <= 0 || cy >= H || cy + h <= 0) return;

        // Clip source Y bounds to avoid off-screen row processing
        int srcYStart = 0;
        int srcYEnd = h;
        if ((flags & BMP_FLIP_V) == 0) {
            if (cy < 0) srcYStart = -cy;
            if (cy + h > H) srcYEnd = H - cy;
        } else {
            if (cy < 0) srcYEnd = h + cy;
            if (cy + h > H) srcYStart = (cy + h) - H;
        }

        for (int py = srcYStart; py < srcYEnd; py++) {
            int drawY = cy + ((flags & BMP_FLIP_V) ? (h - 1 - py) : py);

            for (int bx = 0; bx < bytesPerRow; bx++) {
                uint8_t byteVal = pgm_read_byte(bmp + (py * bytesPerRow) + bx);
                if (!byteVal) continue; // Fast skip empty 8-pixel blocks

                for (int bit = 0; bit < 8; bit++) {
                    int px = bx * 8 + bit;
                    if (px >= w) break;
                    
                    if (byteVal & (1 << (7 - bit))) {
                        int drawX = cx + ((flags & BMP_FLIP_H) ? (w - 1 - px) : px);
                        if (drawX >= 0 && drawX < W) _disp.drawPixel(drawX, drawY);
                    }
                }
            }
        }
    }

    // Tilemap renderer with camera culling
    // mapWidth/mapHeight are in TILES, not pixels. tileW/tileH are pixel dimensions.
    void drawTilemap(int x, int y, const uint8_t* map, int mapW, int mapH, const uint8_t* tileset, int tileW, int tileH, int tilesetBytesPerRow) {
        int cx = x + _camX();
        int cy = y + _camY();

        // Calculate visible tile range
        int startCol = gclamp(-cx / tileW, 0, mapW - 1);
        int startRow = gclamp(-cy / tileH, 0, mapH - 1);
        int endCol   = gclamp((W - cx - 1) / tileW, 0, mapW - 1);
        int endRow   = gclamp((H - cy - 1) / tileH, 0, mapH - 1);

        for (int row = startRow; row <= endRow; row++) {
            for (int col = startCol; col <= endCol; col++) {
                uint8_t tileIdx = pgm_read_byte(map + (row * mapW) + col);
                if (tileIdx == 0) continue; // Assume 0 is empty/transparent

                // Calculate where the tile data starts in the tileset PROGMEM array
                const uint8_t* tileData = tileset + (tileIdx * tileH * tilesetBytesPerRow);
                
                int px = cx + (col * tileW);
                int py = cy + (row * tileH);
                _disp.drawBitmap(px, py, tilesetBytesPerRow, tileH, tileData);
            }
        }
    }

    // ── Emergency flush ───────────────────────────────────────────────────────
    // Force-send the display buffer immediately.  Use only for critical moments
    // where the normal OS draw cycle can't be waited for (e.g., right before
    // deep sleep or a hard reset).
    void flush() { _disp.sendBuffer(); }

    // Enter or exit hardware display power-save mode.
    // 1 = display off (lowest power), 0 = display on.
    void setPowerSave(uint8_t mode) { _disp.setPowerSave(mode); }

    // ── Escape hatch ──────────────────────────────────────────────────────────
    // Returns the raw U8G2 reference for features not covered above.
    // Avoid where possible — coupling your game to U8G2 makes driver swaps harder.
    U8G2& gfx() { return _disp; }

private:
    Console(U8G2& disp, InputManager& input, Sound& sound, SaveManager& save)
        : _disp(disp), _input(input), _sound(sound), _save(save)
    {}

    friend class OS;

    int _camX() const { return (_camera && !_screenSpace) ? _camera->getOffsetX() : 0; }
    int _camY() const { return (_camera && !_screenSpace) ? _camera->getOffsetY() : 0; }

    // ── Draw state stack (internal) ───────────────────────────────────────────
    struct DrawState {
        uint8_t color;
    };
    DrawState _drawStack[4];
    uint8_t _drawStackCount = 0;

    U8G2&         _disp;
    InputManager& _input;
    Sound&        _sound;
    SaveManager&  _save;
    Camera*       _camera = nullptr;
    bool          _screenSpace = false;
};