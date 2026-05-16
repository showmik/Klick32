#pragma once
#include <Arduino.h>
#include <Preferences.h>

// ─── SaveManager ──────────────────────────────────────────────────────────────
// Persistent key-value storage backed by ESP32 NVS (Non-Volatile Storage).
// Data survives power loss and firmware updates.
//
// Lifecycle — managed by OS, not by games:
//   OS calls begin(gameName) when a game is launched → namespace opens.
//   OS calls end()           when a game exits        → namespace closes.
//   Games access it exclusively through Console::save*() / Console::load*().
//
// Namespaces:
//   Each game writes into its own NVS namespace, so games can never clobber
//   each other's data.  The namespace is derived from the game's getName()
//   return value: lowercased, spaces replaced with '_', truncated to 15 chars
//   (the hard NVS limit).
//
//   "Dino Run"  → "dino_run"
//   "Space Shooter 3000" → "space_shooter_3"
//
// Key rules (NVS limits):
//   Keys must be ≤ 15 characters.  The helper names below all comply.
//
// Supported types:
//   uint32_t  — scores, counters, flags packed into bits
//   int32_t   — signed values, offsets
//   float     — physics constants, persistent floats
//   bool      — feature flags, one-time events
//   uint8_t   — small enums, levels 0-255
//
// Convenience shortcut:
//   saveHiScore(n) / loadHiScore()  — uses the reserved key "hi".
//   Always pass a non-zero default to loadHiScore() only if you want a
//   head-start score; otherwise 0 is the correct default.
//
// ─────────────────────────────────────────────────────────────────────────────

class SaveManager {
public:

    // ── Lifecycle (called by OS only) ─────────────────────────────────────────

    // Open the NVS namespace for gameName.
    // Safe to call again with a different name — closes the previous one first.
    void begin(const char* gameName) {
        if (_open) _prefs.end();
        char ns[16];
        _makeNamespace(gameName, ns);
        _prefs.begin(ns, false);   // false = read-write mode
        _open = true;
    }

    // Flush and close the current namespace.  Call on game exit.
    void end() {
        if (_open) { _prefs.end(); _open = false; }
    }

    // ── Write ─────────────────────────────────────────────────────────────────

    void putUInt (const char* key, uint32_t v) { _prefs.putUInt (key, v); }
    void putInt  (const char* key, int32_t  v) { _prefs.putInt  (key, v); }
    void putFloat(const char* key, float    v) { _prefs.putFloat(key, v); }
    void putBool (const char* key, bool     v) { _prefs.putBool (key, v); }
    void putByte (const char* key, uint8_t  v) { _prefs.putUChar(key, v); }
    void putBytes(const char* key, const void* v, size_t len) { _prefs.putBytes(key, v, len); }

    // ── Read ──────────────────────────────────────────────────────────────────

    uint32_t getUInt (const char* key, uint32_t def = 0)      { return _prefs.getUInt (key, def); }
    int32_t  getInt  (const char* key, int32_t  def = 0)      { return _prefs.getInt  (key, def); }
    float    getFloat(const char* key, float    def = 0.0f)   { return _prefs.getFloat(key, def); }
    bool     getBool (const char* key, bool     def = false)   { return _prefs.getBool (key, def); }
    uint8_t  getByte (const char* key, uint8_t  def = 0)      { return _prefs.getUChar(key, def); }
    size_t   getBytes(const char* key, void* buf, size_t maxLen) { return _prefs.getBytes(key, buf, maxLen); }

    // True if the key exists in the current namespace.
    bool hasKey(const char* key) { return _prefs.isKey(key); }

    // ── Hi-score convenience ──────────────────────────────────────────────────
    // Reserved key "hi" — update only when the new score beats the stored one.
    // Returns true when a new hi-score was set.
    bool updateHiScore(uint32_t score) {
        uint32_t stored = loadHiScore();
        if (score > stored) { putUInt("hi", score); return true; }
        return false;
    }

    void     saveHiScore(uint32_t score) { putUInt("hi", score); }
    uint32_t loadHiScore()               { return getUInt("hi", 0); }

    // ── Erase ─────────────────────────────────────────────────────────────────

    // Erase ALL keys in the current game's namespace.
    // Useful for a "clear data" option in a game's pause menu.
    void clearAll() { if (_open) _prefs.clear(); }

    // Remove a single key.
    void remove(const char* key) { if (_open) _prefs.remove(key); }

    // ── Status ────────────────────────────────────────────────────────────────
    bool isOpen() const { return _open; }

private:
    Preferences _prefs;
    bool        _open = false;

    // Derive a valid NVS namespace from a human-readable game name.
    //   • lowercase
    //   • spaces → '_'
    //   • strip any other non-alphanumeric characters except '_'
    //   • truncate to 15 characters (NVS hard limit)
    static void _makeNamespace(const char* name, char out[16]) {
        uint8_t len = 0;
        for (const char* p = name; *p && len < 15; ++p) {
            char c = *p;
            if (c >= 'A' && c <= 'Z') c += 32;     // to lower
            if (c == ' ') c = '_';
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
                out[len++] = c;
            }
        }
        if (len == 0) { out[0] = 'g'; len = 1; }   // fallback: "g"
        out[len] = '\0';
    }
};
