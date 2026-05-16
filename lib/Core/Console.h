#pragma once
#include <U8g2lib.h>
#include "InputManager.h"
#include "Sound.h"
#include "SaveManager.h"
#include "Camera.h"

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
    void sfxJump()      { SFX::jump(_sound);      }
    void sfxDeath()     { SFX::death(_sound);     }
    void sfxPoint()     { SFX::point(_sound);     }
    void sfxMenuNav()   { SFX::menuNav(_sound);   }
    void sfxMenuEnter() { SFX::menuEnter(_sound); }
    void sfxMenuBack()  { SFX::menuBack(_sound);  }

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

    // ── Read ──────────────────────────────────────────────────────────────────
    uint32_t loadUInt (const char* key, uint32_t def = 0)    { return _save.getUInt (key, def); }
    int32_t  loadInt  (const char* key, int32_t  def = 0)    { return _save.getInt  (key, def); }
    float    loadFloat(const char* key, float    def = 0.0f) { return _save.getFloat(key, def); }
    bool     loadBool (const char* key, bool     def = false) { return _save.getBool (key, def); }
    uint8_t  loadByte (const char* key, uint8_t  def = 0)    { return _save.getByte (key, def); }
    size_t   loadBytes(const char* key, void* buf, size_t maxLen) { return _save.getBytes(key, buf, maxLen); }

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

    // ══════════════════════════════════════════════════════════════════════════
    // DRAWING
    // ══════════════════════════════════════════════════════════════════════════

    // color: 0 = clear/black, 1 = set/white, 2 = XOR
    void setDrawColor(uint8_t color)  { _disp.setDrawColor(color); }
    void setFont(const uint8_t* font) { _disp.setFont(font);       }

    // Pixel width of str in the currently-set font.
    int strWidth(const char* str) { return (int)_disp.getStrWidth(str); }

    // ── Pixels and lines ──────────────────────────────────────────────────────
    void drawPixel(int x, int y)                    { _disp.drawPixel(x + _camX(), y + _camY());           }
    void drawHLine(int x, int y, int w)             { _disp.drawHLine(x + _camX(), y + _camY(), w);        }
    void drawVLine(int x, int y, int h)             { _disp.drawVLine(x + _camX(), y + _camY(), h);        }
    void drawLine (int x1, int y1, int x2, int y2) { _disp.drawLine(x1 + _camX(), y1 + _camY(), x2 + _camX(), y2 + _camY()); }

    // ── Rectangles ────────────────────────────────────────────────────────────
    void drawFrame (int x, int y, int w, int h)        { _disp.drawFrame(x + _camX(), y + _camY(), w, h);     }
    void drawBox   (int x, int y, int w, int h)        { _disp.drawBox(x + _camX(), y + _camY(), w, h);       }
    void drawRFrame(int x, int y, int w, int h, int r) { _disp.drawRFrame(x + _camX(), y + _camY(), w, h, r); }
    void drawRBox  (int x, int y, int w, int h, int r) { _disp.drawRBox(x + _camX(), y + _camY(), w, h, r);   }

    // ── Circles ───────────────────────────────────────────────────────────────
    void drawCircle(int cx, int cy, int r) { _disp.drawCircle(cx + _camX(), cy + _camY(), r); }
    void drawDisc  (int cx, int cy, int r) { _disp.drawDisc(cx + _camX(), cy + _camY(), r);   }

    // ── Text ──────────────────────────────────────────────────────────────────
    // y is the baseline, not the top of the character.
    void drawStr(int x, int y, const char* str) { _disp.drawStr(x + _camX(), y + _camY(), str); }

    // ── Bitmaps ───────────────────────────────────────────────────────────────
    // bytesPerRow = ceil(spriteWidthPx / 8).
    //   8 px wide  → bytesPerRow = 1
    //   16 px wide → bytesPerRow = 2
    // bmp must be in PROGMEM.
    void drawBitmap(int x, int y, int bytesPerRow, int h, const uint8_t* bmp) {
        _disp.drawBitmap(x + _camX(), y + _camY(), bytesPerRow, h, bmp);
    }

    // ── Escape hatch ──────────────────────────────────────────────────────────
    // Returns the raw U8G2 reference for features not covered above.
    // Avoid where possible — coupling your game to U8G2 makes driver swaps harder.
    U8G2& gfx() { return _disp; }

private:
    Console(U8G2& disp, InputManager& input, Sound& sound, SaveManager& save)
        : _disp(disp), _input(input), _sound(sound), _save(save)
    {}

    friend class OS;

    int _camX() const { return _camera ? _camera->getOffsetX() : 0; }
    int _camY() const { return _camera ? _camera->getOffsetY() : 0; }

    U8G2&         _disp;
    InputManager& _input;
    Sound&        _sound;
    SaveManager&  _save;
    Camera*       _camera = nullptr;
};