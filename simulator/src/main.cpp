#include <SDL.h>
#undef main
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include "OS.h"
#include "Config.h"
#include "Arduino.h"

ESPMock ESP;
SerialMock Serial;
WireMock Wire;

extern "C" {
    // Dummy display callback to prevent u8x8 from dereferencing a NULL pointer
    uint8_t u8x8_dummy_display_cb(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
        return 1;
    }

    // Provide the missing u8g2_SetupBitmap implementation that U8G2_BITMAP expects
    void u8g2_SetupBitmap(u8g2_t *u8g2, const u8g2_cb_t *rotation, uint16_t pixel_width, uint16_t pixel_height) {
        u8g2->u8x8.display_cb = u8x8_dummy_display_cb;
        u8g2->u8x8.cad_cb = u8x8_dummy_display_cb; // Just in case
        u8g2->u8x8.byte_cb = u8x8_dummy_display_cb;
        u8g2->u8x8.gpio_and_delay_cb = u8x8_dummy_display_cb;
        
        static u8x8_display_info_t dummy_display_info;
        memset(&dummy_display_info, 0, sizeof(dummy_display_info));
        dummy_display_info.tile_width = pixel_width / 8;
        dummy_display_info.tile_height = pixel_height / 8;
        dummy_display_info.pixel_width = pixel_width;
        dummy_display_info.pixel_height = pixel_height;
        u8g2->u8x8.display_info = &dummy_display_info;
        
        // Provide a valid buffer immediately so _disp.begin() doesn't memset a nullptr!
        static uint8_t initial_sim_buf[1024];
        u8g2_SetupBuffer(u8g2, initial_sim_buf, pixel_height / 8, u8g2_ll_hvline_vertical_top_lsb, rotation);
    }
}

OS* g_os = nullptr;
bool g_simPins[256] = {false};

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;

bool g_quit = false;

// We need to implement a hook into the U8G2 buffer rendering
// The U8G2_SIMULATOR class maintains _buf[1024].
void RenderDisplay(uint8_t* buf) {
    if (!buf) return;
    
    uint32_t pixels[128 * 64];
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x++) {
            int byteIdx = (y / 8) * 128 + x;
            int bitIdx = y % 8;
            bool isOn = (buf[byteIdx] & (1 << bitIdx)) != 0;
            pixels[y * 128 + x] = isOn ? 0xFFFFFFFF : 0xFF000000;
        }
    }
    
    SDL_UpdateTexture(texture, NULL, pixels, 128 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

// Injected into Arduino.h mock
void SimInjectKey(int sdlKey, bool down) {
    uint8_t pin = 255;
    switch (sdlKey) {
        case SDLK_UP:     pin = PIN_UP; break;
        case SDLK_DOWN:   pin = PIN_DOWN; break;
        case SDLK_LEFT:   pin = PIN_LEFT; break;
        case SDLK_RIGHT:  pin = PIN_RIGHT; break;
        case SDLK_z:      pin = PIN_BTN_A; break;
        case SDLK_x:      pin = PIN_BTN_B; break;
        case SDLK_RETURN: pin = PIN_MENU1; break;
        case SDLK_ESCAPE: pin = PIN_MENU2; break;
        default: break;
    }
    if (pin != 255) {
        g_simPins[pin] = down;
    }
}

void PumpEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            g_quit = true;
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            bool down = (e.type == SDL_KEYDOWN);
            SimInjectKey(e.key.keysym.sym, down);
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
            MessageBoxA(NULL, SDL_GetError(), "SDL_Init Failed", MB_OK | MB_ICONERROR);
            return 1;
        }

        window = SDL_CreateWindow("Klick32 Simulator", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            128 * 4, 64 * 4, SDL_WINDOW_SHOWN);
            
        if (!window) {
            return 1;
        }
            
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer) {
            return 1;
        }

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 128, 64);
        if (!texture) {
            return 1;
        }

        OS os;
        g_os = &os;
        os.begin();

    std::thread osThread([&]() {
        os.run();
    });

    extern uint8_t* g_simDisplayBuffer;

    while (!g_quit) {
        PumpEvents();
        RenderDisplay(g_simDisplayBuffer);
        SDL_Delay(16);
    }
    
        osThread.detach();

        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        return 0;
    } catch (const std::exception& e) {
        MessageBoxA(NULL, e.what(), "Unhandled Exception", MB_OK | MB_ICONERROR);
        return 1;
    } catch (...) {
        MessageBoxA(NULL, "An unknown exception occurred.", "Fatal Error", MB_OK | MB_ICONERROR);
        return 1;
    }
}
