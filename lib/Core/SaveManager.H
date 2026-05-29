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
//   string    — player names, short labels (≤ 15 chars)
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
        if (_open) { 
            commit(); // Flush cache to physical flash
            _prefs.end(); 
            _open = false; 
            _cacheCount = 0;
        }
    }

    // Force flush all dirty variables to physical flash (ESP32 NVS)
    void commit() {
        for (int i = 0; i < _cacheCount; i++) {
            if (_cache[i].isDirty) {
                switch (_cache[i].type) {
                    case T_UINT:  _prefs.putUInt (_cache[i].key, _cache[i].data.u); break;
                    case T_INT:   _prefs.putInt  (_cache[i].key, _cache[i].data.i); break;
                    case T_FLOAT: _prefs.putFloat(_cache[i].key, _cache[i].data.f); break;
                    case T_BOOL:  _prefs.putBool (_cache[i].key, _cache[i].data.b); break;
                    case T_BYTE:  _prefs.putUChar(_cache[i].key, _cache[i].data.u8); break;
                    default: break;
                }
                _cache[i].isDirty = false;
            }
        }
    }

    // ── Write ─────────────────────────────────────────────────────────────────

    void putUInt (const char* key, uint32_t v) { _setCache(key, v, T_UINT); }
    void putInt  (const char* key, int32_t  v) { _setCache(key, v, T_INT); }
    void putFloat(const char* key, float    v) { _setCache(key, v, T_FLOAT); }
    void putBool (const char* key, bool     v) { _setCache(key, v, T_BOOL); }
    void putByte (const char* key, uint8_t  v) { _setCache(key, v, T_BYTE); }
    
    // Strings and raw bytes bypass the cache directly to flash
    void putBytes(const char* key, const void* v, size_t len) { _prefs.putBytes(key, v, len); }
    void putStr  (const char* key, const char* v)              { _prefs.putString(key, v); }

    // ── Read ──────────────────────────────────────────────────────────────────

    uint32_t getUInt (const char* key, uint32_t def = 0)      { return _getCache(key, def, T_UINT).u; }
    int32_t  getInt  (const char* key, int32_t  def = 0)      { return _getCache(key, def, T_INT).i; }
    float    getFloat(const char* key, float    def = 0.0f)   { return _getCache(key, def, T_FLOAT).f; }
    bool     getBool (const char* key, bool     def = false)  { return _getCache(key, def, T_BOOL).b; }
    uint8_t  getByte (const char* key, uint8_t  def = 0)      { return _getCache(key, def, T_BYTE).u8; }
    
    // Bypasses cache
    size_t   getBytes(const char* key, void* buf, size_t maxLen) { return _prefs.getBytes(key, buf, maxLen); }

    // Read a string. Returns actual length written (excluding null terminator), 0 if key not found.
    size_t   getStr(const char* key, char* buf, size_t bufSize, const char* def = "") {
        String s = _prefs.getString(key, def);
        size_t len = s.length();
        if (len >= bufSize) len = bufSize - 1;
        memcpy(buf, s.c_str(), len);
        buf[len] = '\0';
        return len;
    }

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
    void clearAll() { 
        if (_open) {
            _prefs.clear(); 
            _cacheCount = 0; // Wipe cache
        }
    }

    // Remove a single key.
    void remove(const char* key) { 
        if (_open) {
            _prefs.remove(key); 
            // Mark inactive in cache
            CacheEntry* e = _findCache(key);
            if (e) e->type = T_NONE;
        }
    }

    // ── Status ────────────────────────────────────────────────────────────────
    bool isOpen() const { return _open; }

private:
    Preferences _prefs;
    bool        _open = false;

    // ── Wear-Leveling RAM Cache ──
    enum CacheType { T_NONE, T_UINT, T_INT, T_FLOAT, T_BOOL, T_BYTE };
    union CacheData {
        uint32_t u;
        int32_t  i;
        float    f;
        bool     b;
        uint8_t  u8;
        
        // Constructor for assignment logic
        CacheData() { u = 0; }
        CacheData(uint32_t val) { u = val; }
        CacheData(int32_t val) { u = 0; i = val; }
        CacheData(float val) { u = 0; f = val; }
        CacheData(bool val) { u = 0; b = val; }
        CacheData(uint8_t val) { u = 0; u8 = val; }
        
        bool operator==(const CacheData& o) const { return u == o.u; }
    };
    
    struct CacheEntry {
        char key[16];
        CacheData data;
        CacheType type;
        bool isDirty;
    };
    
    static constexpr int MAX_CACHE = 16;
    CacheEntry _cache[MAX_CACHE];
    uint8_t _cacheCount = 0;

    CacheEntry* _findCache(const char* key) {
        for (int i=0; i<_cacheCount; i++) {
            if (_cache[i].type != T_NONE && strcmp(_cache[i].key, key) == 0) return &_cache[i];
        }
        return nullptr;
    }

    template<typename T>
    void _setCache(const char* key, T value, CacheType type) {
        CacheData newData(value);
        CacheEntry* e = _findCache(key);
        if (e) {
            if (e->type != type || !(e->data == newData)) {
                e->data = newData;
                e->type = type;
                e->isDirty = true;
            }
            return;
        }
        
        // Not in cache, try to add it
        if (_cacheCount < MAX_CACHE) {
            e = &_cache[_cacheCount++];
            strncpy(e->key, key, 15);
            e->key[15] = '\0';
            e->data = newData;
            e->type = type;
            e->isDirty = true;
            return;
        }
        
        // Cache full, write-through to physical flash
        if (type == T_UINT) _prefs.putUInt(key, newData.u);
        else if (type == T_INT) _prefs.putInt(key, newData.i);
        else if (type == T_FLOAT) _prefs.putFloat(key, newData.f);
        else if (type == T_BOOL) _prefs.putBool(key, newData.b);
        else if (type == T_BYTE) _prefs.putUChar(key, newData.u8);
    }

    template<typename T>
    CacheData _getCache(const char* key, T defVal, CacheType type) {
        CacheEntry* e = _findCache(key);
        if (e && e->type == type) return e->data;
        
        // Cache miss: Load from physical flash and cache it
        CacheData data(defVal);
        if (type == T_UINT) data.u = _prefs.getUInt(key, defVal);
        else if (type == T_INT) data.i = _prefs.getInt(key, defVal);
        else if (type == T_FLOAT) data.f = _prefs.getFloat(key, defVal);
        else if (type == T_BOOL) data.b = _prefs.getBool(key, defVal);
        else if (type == T_BYTE) data.u8 = _prefs.getUChar(key, defVal);
        
        if (_cacheCount < MAX_CACHE) {
            e = &_cache[_cacheCount++];
            strncpy(e->key, key, 15);
            e->key[15] = '\0';
            e->data = data;
            e->type = type;
            e->isDirty = false; // Loaded from flash, not dirty!
        }
        return data;
    }

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
