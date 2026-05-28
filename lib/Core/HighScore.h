#pragma once
#include "SaveManager.h"
#include <stdint.h>
#include <string.h>

// ─── HighScore ───────────────────────────────────────────────────────────────
// Manages saving, loading, and sorting high scores via the NVS SaveManager.
// ─────────────────────────────────────────────────────────────────────────────
class HighScore {
public:
    HighScore() = default;

    // Load top N scores from SaveManager. slots must be <= 5.
    void begin(SaveManager& save, const char* keyPrefix = "hi", uint8_t slots = 1) {
        _save = &save;
        _slots = (slots > 5) ? 5 : slots;
        strncpy(_prefix, keyPrefix, 15);
        _prefix[15] = '\0';
        
        for (uint8_t i = 0; i < _slots; i++) {
            char key[20];
            snprintf(key, sizeof(key), "%s%d", _prefix, i);
            _scores[i] = _save->load(key, 0);
        }
    }

    // Return the highest score (rank 0)
    uint32_t getBest() const {
        return _scores[0];
    }

    // Return score at a specific rank (0 is best)
    uint32_t get(uint8_t rank) const {
        if (rank >= _slots) return 0;
        return _scores[rank];
    }

    // Check if the given score beats any high score
    bool isHighScore(uint32_t score) const {
        if (_slots == 0) return false;
        return score > _scores[_slots - 1];
    }

    // Submit a new score. If it qualifies, it is inserted and saved.
    // Returns the rank it achieved (0 to slots-1), or -1 if it didn't make the cut.
    int submit(uint32_t score) {
        if (!isHighScore(score)) return -1;
        
        int rank = _slots - 1;
        while (rank >= 0 && score > _scores[rank]) {
            rank--;
        }
        rank++; // The slot we will insert into
        
        // Shift lower scores down
        for (int i = _slots - 1; i > rank; i--) {
            _scores[i] = _scores[i - 1];
        }
        
        _scores[rank] = score;
        
        // Save to NVS
        for (uint8_t i = rank; i < _slots; i++) {
            char key[20];
            snprintf(key, sizeof(key), "%s%d", _prefix, i);
            _save->save(key, _scores[i]);
        }
        
        return rank;
    }

private:
    SaveManager* _save = nullptr;
    uint32_t _scores[5] = {0};
    uint8_t _slots = 1;
    char _prefix[16] = {0};
};
