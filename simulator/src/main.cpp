#include <SDL.h>
#undef main
#include <iostream>
#include <windows.h>
#include <stdio.h>
#include <mutex>
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
SDL_GameController* controller = nullptr;

bool g_quit = false;

extern uint8_t* g_simDisplayBuffer;
uint8_t g_simFrontBuffer[1024];
std::mutex g_displayMutex;

void sim_commit_buffer() {
    std::lock_guard<std::mutex> lock(g_displayMutex);
    if (g_simDisplayBuffer) {
        memcpy(g_simFrontBuffer, g_simDisplayBuffer, 1024);
    }
}

void RenderDisplay() {
    uint32_t pixels[128 * 64];
    
    {
        std::lock_guard<std::mutex> lock(g_displayMutex);
        for (int y = 0; y < 64; y++) {
            for (int x = 0; x < 128; x++) {
                int byteIdx = (y / 8) * 128 + x;
                int bitIdx = y % 8;
                bool isOn = (g_simFrontBuffer[byteIdx] & (1 << bitIdx)) != 0;
                pixels[y * 128 + x] = isOn ? 0xFFFFFFFF : 0xFF000000;
            }
        }
    }
    
    SDL_UpdateTexture(texture, NULL, pixels, 128 * sizeof(uint32_t));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
}

void SaveScreenshot() {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, 128, 64, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return;
    
    std::lock_guard<std::mutex> lock(g_displayMutex);
    uint32_t* pixels = (uint32_t*)surface->pixels;
    for (int y = 0; y < 64; y++) {
        for (int x = 0; x < 128; x++) {
            int byteIdx = (y / 8) * 128 + x;
            int bitIdx = y % 8;
            bool isOn = (g_simFrontBuffer[byteIdx] & (1 << bitIdx)) != 0;
            pixels[y * 128 + x] = isOn ? 0xFFFFFFFF : 0xFF000000;
        }
    }
    
    char filename[64];
    sprintf(filename, "screenshot_%u.bmp", SDL_GetTicks());
    SDL_SaveBMP(surface, filename);
    SDL_FreeSurface(surface);
    std::cout << "Screenshot saved to " << filename << std::endl;
}

// Injected into Arduino.h mock
void SimInjectKey(int sdlKey, bool down) {
    uint8_t pin = 255;
    switch (sdlKey) {
        case SDLK_UP:
        case SDLK_w:      pin = PIN_UP; break;
        case SDLK_DOWN:
        case SDLK_s:      pin = PIN_DOWN; break;
        case SDLK_LEFT:
        case SDLK_a:      pin = PIN_LEFT; break;
        case SDLK_RIGHT:
        case SDLK_d:      pin = PIN_RIGHT; break;
        case SDLK_z:
        case SDLK_j:      pin = PIN_BTN_A; break;
        case SDLK_x:
        case SDLK_k:      pin = PIN_BTN_B; break;
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
            if (down && e.key.keysym.sym == SDLK_F12) {
                SaveScreenshot();
            } else {
                SimInjectKey(e.key.keysym.sym, down);
            }
        } else if (e.type == SDL_CONTROLLERBUTTONDOWN || e.type == SDL_CONTROLLERBUTTONUP) {
            bool down = (e.type == SDL_CONTROLLERBUTTONDOWN);
            int key = 0;
            switch(e.cbutton.button) {
                case SDL_CONTROLLER_BUTTON_DPAD_UP:    key = SDLK_UP; break;
                case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  key = SDLK_DOWN; break;
                case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  key = SDLK_LEFT; break;
                case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: key = SDLK_RIGHT; break;
                case SDL_CONTROLLER_BUTTON_A:          key = SDLK_z; break;
                case SDL_CONTROLLER_BUTTON_B:          key = SDLK_x; break;
                case SDL_CONTROLLER_BUTTON_X:          key = SDLK_x; break;
                case SDL_CONTROLLER_BUTTON_START:      key = SDLK_RETURN; break;
                case SDL_CONTROLLER_BUTTON_BACK:       key = SDLK_ESCAPE; break;
            }
            if (key != 0) {
                SimInjectKey(key, down);
            }
        } else if (e.type == SDL_CONTROLLERDEVICEADDED) {
            if (!controller) {
                controller = SDL_GameControllerOpen(e.cdevice.which);
            }
        } else if (e.type == SDL_CONTROLLERDEVICEREMOVED) {
            if (controller && e.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
                SDL_GameControllerClose(controller);
                controller = nullptr;
            }
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
            MessageBoxA(NULL, SDL_GetError(), "SDL_Init Failed", MB_OK | MB_ICONERROR);
            return 1;
        }

        window = SDL_CreateWindow("Klick32 Simulator", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
            128 * 4, 64 * 4, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
            
        if (!window) {
            return 1;
        }
            
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        if (!renderer) {
            return 1;
        }
        SDL_RenderSetLogicalSize(renderer, 128, 64);

        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
            SDL_TEXTUREACCESS_STREAMING, 128, 64);
        if (!texture) {
            return 1;
        }

        // Initialize front buffer to black
        memset(g_simFrontBuffer, 0, sizeof(g_simFrontBuffer));

        OS os;
        g_os = &os;
        os.begin();

        std::thread osThread([&]() {
            os.run();
        });

        while (!g_quit) {
            uint32_t startTick = SDL_GetTicks();
            PumpEvents();
            RenderDisplay();
            uint32_t elapsed = SDL_GetTicks() - startTick;
            if (elapsed < 16) {
                SDL_Delay(16 - elapsed);
            }
        }
        
        osThread.join();

        if (controller) SDL_GameControllerClose(controller);

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
