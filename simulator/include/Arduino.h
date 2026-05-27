#pragma once
#ifndef ARDUINO_H
#define ARDUINO_H

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <chrono>
#include <thread>
#include <random>
#include <string>

// Arduino String Mock
typedef std::string String;

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

// ─── PROGMEM Mocking ──────────────────────────────────────────────────────────
#define PROGMEM
#define IRAM_ATTR
#define pgm_read_byte(addr) (*(const uint8_t *)(addr))
#define pgm_read_word(addr) (*(const uint16_t *)(addr))
#define pgm_read_dword(addr) (*(const uint32_t *)(addr))
#define pgm_read_ptr(addr) (*(const void * const *)(addr))

// ─── ESP32 FreeRTOS & SDK Mocking ─────────────────────────────────────────────
#define configMAX_PRIORITIES 25
typedef void* TaskHandle_t;
#define xTaskCreatePinnedToCore(func, name, stack, params, prio, handle, core) \
    do { \
        std::thread([func]() { func(nullptr); }).detach(); \
    } while(0)
#define vTaskDelay(ticks) std::this_thread::sleep_for(std::chrono::milliseconds(ticks))
#define pdMS_TO_TICKS(ms) (ms)
#define esp_timer_get_time() micros()
#define esp_rom_delay_us(us) std::this_thread::sleep_for(std::chrono::microseconds(us))
inline void esp_deep_sleep_start() { exit(0); }

// ─── Timing ───────────────────────────────────────────────────────────────────
inline uint32_t millis() {
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now() - start).count());
}

inline uint32_t micros() {
    using namespace std::chrono;
    static auto start = steady_clock::now();
    return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - start).count());
}

inline void delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ─── Random ───────────────────────────────────────────────────────────────────
inline uint32_t esp_random() {
    static std::mt19937 rng(std::random_device{}());
    return rng();
}
inline void randomSeed(uint32_t seed) {
    srand(seed);
}
inline long random(long min, long max) {
    if (min >= max) return min;
    return min + (rand() % (max - min));
}
inline long random(long max) {
    if (max == 0) return 0;
    return rand() % max;
}

// ─── GPIO Mocking ─────────────────────────────────────────────────────────────
#define OUTPUT 0
#define INPUT 1
#define INPUT_PULLUP 2
#define LOW 0
#define HIGH 1

extern bool g_simPins[256]; // true = pressed (LOW), false = unpressed (HIGH)

inline void pinMode(uint8_t pin, uint8_t mode) {}
inline void digitalWrite(uint8_t pin, uint8_t val) {}
inline int digitalRead(uint8_t pin) { return g_simPins[pin] ? LOW : HIGH; }
inline void analogReadResolution(uint8_t bits) {}
inline uint32_t analogReadMilliVolts(uint8_t pin) { return 3700; }

// ─── LEDC Mocking ─────────────────────────────────────────────────────────────
inline void ledcAttach(uint8_t pin, uint32_t freq, uint8_t resolution) {}
inline void ledcWrite(uint8_t pin, uint32_t duty) {}

// ─── ESP / System Mocking ─────────────────────────────────────────────────────
struct ESPMock {
    uint32_t getFreeHeap() { return 8000000; }
    uint32_t getMaxAllocHeap() { return 4000000; }
};
extern ESPMock ESP;

// ─── Serial Mocking ───────────────────────────────────────────────────────────
#include <iostream>
struct SerialMock {
    void begin(uint32_t baud) {}
    void print(const char* s) { std::cout << s; }
    void println(const char* s) { std::cout << s << std::endl; }
    void print(int n) { std::cout << n; }
    void println(int n) { std::cout << n << std::endl; }
};
extern SerialMock Serial;

// ─── Wire Mocking ─────────────────────────────────────────────────────────────
struct WireMock {
    void begin(int sda, int scl) {}
};
extern WireMock Wire;

#endif
