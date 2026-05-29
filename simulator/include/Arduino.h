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
inline std::chrono::steady_clock::time_point& GetSimStartTime() {
    static auto start = std::chrono::steady_clock::now();
    return start;
}

inline uint32_t millis() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<milliseconds>(steady_clock::now() - GetSimStartTime()).count());
}

inline uint32_t micros() {
    using namespace std::chrono;
    return static_cast<uint32_t>(duration_cast<microseconds>(steady_clock::now() - GetSimStartTime()).count());
}

inline void delay(uint32_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ─── Random ───────────────────────────────────────────────────────────────────
inline std::mt19937& GetSimRNG() {
    static std::mt19937 rng(std::random_device{}());
    return rng;
}

inline uint32_t esp_random() {
    return GetSimRNG()();
}
inline void randomSeed(uint32_t seed) {
    GetSimRNG().seed(seed);
}
inline long random(long min, long max) {
    if (min >= max) return min;
    return min + (GetSimRNG()() % (max - min));
}
inline long random(long max) {
    if (max <= 0) return 0;
    return GetSimRNG()() % max;
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
#include <fstream>
#include <stdarg.h>
struct SerialMock {
    std::ofstream logFile;
    void begin(uint32_t baud) {
        logFile.open("simulator_log.txt", std::ios::app);
        if (logFile.is_open()) {
            logFile << "--- Simulator Started ---" << std::endl;
        }
    }
    void print(const char* s) { 
        std::cout << s; 
        if (logFile.is_open()) { logFile << s; logFile.flush(); }
    }
    void println(const char* s) { 
        std::cout << s << std::endl; 
        if (logFile.is_open()) { logFile << s << std::endl; logFile.flush(); }
    }
    void print(int n) { 
        std::cout << n; 
        if (logFile.is_open()) { logFile << n; logFile.flush(); }
    }
    void println(int n) { 
        std::cout << n << std::endl; 
        if (logFile.is_open()) { logFile << n << std::endl; logFile.flush(); }
    }
    void printf(const char* fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        print(buf);
    }
};
extern SerialMock Serial;

// ─── Wire Mocking ─────────────────────────────────────────────────────────────
struct WireMock {
    void begin(int sda, int scl) {}
};
extern WireMock Wire;

#endif
