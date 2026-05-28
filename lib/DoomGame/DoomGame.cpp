#include "DoomGame.h"
#include "GameRegistry.h"
#include <cmath>
#include <cstdint>
#include <stdlib.h>

#define MAP_WIDTH 12
#define MAP_HEIGHT 12

static const uint8_t initial_map[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,6,0,0,0,0,1},
    {1,0,2,2,0,0,0,0,3,3,0,1},
    {1,0,2,0,0,5,0,0,0,3,0,1},
    {1,0,0,0,0,0,5,0,0,0,0,1},
    {1,0,0,5,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,4,4,0,0,0,1},
    {1,0,0,0,0,0,4,9,0,0,0,1},
    {1,0,3,3,0,0,0,0,0,0,0,1},
    {1,0,3,0,0,5,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1}
};

static const uint8_t tex_skull[8] = {
    0b00111100,
    0b01111110,
    0b11011011,
    0b11111111,
    0b01111110,
    0b00100100,
    0b00111100,
    0b00000000
};

static const uint8_t tex_skull_mask[8] = {
    0b01111110,
    0b11111111,
    0b11111111,
    0b11111111,
    0b11111111,
    0b01111110,
    0b01111110,
    0b00000000
};

static const uint8_t tex_medkit[8] = {
    0b00000000,
    0b00011000,
    0b00011000,
    0b01111110,
    0b01111110,
    0b00011000,
    0b00011000,
    0b00000000
};

static const uint8_t tex_medkit_mask[8] = {
    0b00000000,
    0b00111100,
    0b00111100,
    0b11111111,
    0b11111111,
    0b00111100,
    0b00111100,
    0b00000000
};

// =============================================================================
// DoomTitleScene
// =============================================================================

void DoomTitleScene::onEnter(Console& ctx) {
    frame_counter = 0;
}

void DoomTitleScene::update(Console& ctx, SceneManager& sm, float dt) {
    frame_counter++;
    if (ctx.justPressed(Btn::A)) {
        ctx.sfxMenuEnter();
        sm.emit(ctx, Event::CUSTOM_1); // Play
    }
    if (ctx.justPressed(Btn::MENU1)) sm.emit(ctx, Event::QUIT);
}

void DoomTitleScene::draw(Console& ctx) {
    // Draw huge skull in the background
    int sx = Console::W / 2 - 16;
    int sy = Console::H / 2 - 24;
    ctx.setDrawColor(Console::COLOR_WHITE);
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (tex_skull[y] & (1 << (7 - x))) {
                ctx.drawBox(sx + x * 4, sy + y * 4, 4, 4);
            }
        }
    }
    
    // Draw pulsing title overlay
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawBox(0, Console::H / 2 + 5, Console::W, 15); // Clear text area
    ctx.setDrawColor(Console::COLOR_WHITE);
    
    ctx.setFont(u8g2_font_6x10_tf);
    ctx.drawStrCentered(Console::H / 2 + 15, "DOOM 1-BIT");
    
    if (_data->hiScore > 0) {
        ctx.setFont(u8g2_font_4x6_tf);
        ctx.drawPrintfCentered(Console::H - 6, "HI: %d", _data->hiScore);
    }
    
    if (frame_counter % 30 < 15) {
        ctx.setFont(u8g2_font_4x6_tf);
        ctx.drawStrCentered(Console::H - 15, "PRESS A");
    }
}

// =============================================================================
// DoomWinScene
// =============================================================================

void DoomWinScene::onEnter(Console& ctx) {
    if (_data->score > _data->hiScore) {
        _data->hiScore = _data->score;
        ctx.saveHiScore(_data->hiScore);
    }
}

void DoomWinScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::A) || ctx.justPressed(Btn::B)) sm.emit(ctx, Event::CUSTOM_2);
    if (ctx.justPressed(Btn::MENU1)) sm.emit(ctx, Event::QUIT);
}

void DoomWinScene::draw(Console& ctx) {
    ctx.setFont(u8g2_font_6x10_tf);
    if (_data->hp <= 0) {
        ctx.drawStrCentered(20, "YOU DIED!");
    } else {
        ctx.drawStrCentered(20, "YOU ESCAPED!");
    }
    ctx.drawPrintfCentered(35, "SCORE: %d", _data->score);
    ctx.drawPrintfCentered(50, "HP: %d AMMO: %d", _data->hp, _data->ammo);
}

// =============================================================================
// DoomPlayScene
// =============================================================================

void DoomPlayScene::onEnter(Console& ctx) {
    player_x = 1.5f;
    player_y = 1.5f;
    player_dir = 0.0f;
    weapon_bob = 0.0f;
    fire_timer = 0;
    frame_counter = 0;
    damage_timer = 0;
    show_minimap = false;
    _data->score = 0;
    _data->hp = 100;
    _data->ammo = 20;
    
    num_sprites = 0;
    for (int y = 0; y < MAP_HEIGHT; y++) {
        for (int x = 0; x < MAP_WIDTH; x++) {
            uint8_t val = initial_map[y][x];
            if (val == 5) { // enemy
                sprites[num_sprites++] = { (float)x + 0.5f, (float)y + 0.5f, 1, true, 0.0f };
                map[y][x] = 0;
            } else if (val == 6) { // medkit
                sprites[num_sprites++] = { (float)x + 0.5f, (float)y + 0.5f, 2, true, 0.0f };
                map[y][x] = 0;
            } else {
                map[y][x] = val;
            }
        }
    }
}

void DoomPlayScene::update(Console& ctx, SceneManager& sm, float dt) {
    if (ctx.justPressed(Btn::MENU1)) { sm.emit(ctx, Event::QUIT); return; }
    if (ctx.justPressed(Btn::B)) { show_minimap = !show_minimap; }
    
    if (dt > 0.1f) dt = 0.1f;
    frame_counter++;
    
    if (damage_timer > 0) damage_timer--;
    if (fire_timer > 0) fire_timer--;

    float move_speed = 3.0f * dt;
    float rot_speed = 2.0f * dt;
    bool moved = false;

    if (ctx.pressed(Btn::LEFT)) player_dir -= rot_speed;
    if (ctx.pressed(Btn::RIGHT)) player_dir += rot_speed;

    if (ctx.pressed(Btn::UP)) {
        float nx = player_x + std::cos(player_dir) * move_speed;
        float ny = player_y + std::sin(player_dir) * move_speed;
        if (map[(int)ny][(int)player_x] == 0 || map[(int)ny][(int)player_x] == 9) player_y = ny;
        if (map[(int)player_y][(int)nx] == 0 || map[(int)player_y][(int)nx] == 9) player_x = nx;
        moved = true;
    }
    if (ctx.pressed(Btn::DOWN)) {
        float nx = player_x - std::cos(player_dir) * move_speed;
        float ny = player_y - std::sin(player_dir) * move_speed;
        if (map[(int)ny][(int)player_x] == 0 || map[(int)ny][(int)player_x] == 9) player_y = ny;
        if (map[(int)player_y][(int)nx] == 0 || map[(int)player_y][(int)nx] == 9) player_x = nx;
        moved = true;
    }

    if (moved) {
        weapon_bob += 10.0f * dt;
        if (weapon_bob > 3.14159f * 2.0f) weapon_bob -= 3.14159f * 2.0f;
    }

    // Player Shooting
    if (ctx.justPressed(Btn::A) && fire_timer == 0 && _data->ammo > 0) {
        fire_timer = 10;
        _data->ammo--;
        ctx.beep(400, 30);
        
        for (int i = 0; i < num_sprites; i++) {
            if (!sprites[i].active || sprites[i].type != 1) continue;
            
            float dx = sprites[i].x - player_x;
            float dy = sprites[i].y - player_y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            float angle = std::atan2(dy, dx) - player_dir;
            while (angle < -3.14159f) angle += 2.0f * 3.14159f;
            while (angle > 3.14159f) angle -= 2.0f * 3.14159f;
            
            if (std::abs(angle) < 0.2f && dist < 8.0f) {
                sprites[i].active = false;
                _data->score += 100;
                ctx.beep(800, 50);
                
                for (int p = 0; p < 10; p++) {
                    float a = (rand() % 360) * 3.14159f / 180.0f;
                    float s = (rand() % 100) / 50.0f;
                    _particles->spawnPixel(Console::W/2, Console::H/2 - 10, std::cos(a)*s, std::sin(a)*s, 10 + rand()%10);
                }
                break;
            }
        }
    }
    
    // Enemy Attacks & Pickups
    for (int i = 0; i < num_sprites; i++) {
        if (!sprites[i].active) continue;
        float dx = sprites[i].x - player_x;
        float dy = sprites[i].y - player_y;
        float dist = std::sqrt(dx*dx + dy*dy);
        
        if (sprites[i].type == 2 && dist < 0.5f) { // Medkit
            _data->hp += 25;
            if (_data->hp > 100) _data->hp = 100;
            sprites[i].active = false;
            ctx.sfxPoint();
        }
        else if (sprites[i].type == 1 && dist < 4.0f) { // Enemy attack range
            if ((rand() % 100) < 2) { // 2% chance per frame to attack
                _data->hp -= 10;
                damage_timer = 5;
                ctx.beep(150, 100);
                if (_data->hp <= 0) {
                    _data->hp = 0;
                    sm.emit(ctx, Event::CUSTOM_3); // Game Over (WinScene handles death text)
                }
            }
        }
    }
    
    if (map[(int)player_y][(int)player_x] == 9) {
        ctx.sfxPoint();
        sm.emit(ctx, Event::CUSTOM_3); // Win
    }
}

void DoomPlayScene::draw(Console& ctx) {
    // Room Illumination Flash on Fire, Screen Shake on Damage
    int shakeX = 0, shakeY = 0;
    if (damage_timer > 0) {
        shakeX = (rand() % 5) - 2;
        shakeY = (rand() % 5) - 2;
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawDitherBox(0, 0, Console::W, Console::H, 2);
    } else if (fire_timer > 8) {
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawDitherBox(0, 0, Console::W, Console::H, 2);
    } else {
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(0, 0, Console::W, Console::H);
    }
    
    float dirX = std::cos(player_dir);
    float dirY = std::sin(player_dir);
    float planeX = -dirY * 0.66f;
    float planeY = dirX * 0.66f;
    
    float zBuffer[128]; 
    int lastMapX = -1;
    int lastMapY = -1;
    
    int viewBobY = static_cast<int>(std::cos(weapon_bob * 2.0f) * 2.0f);
    
    for (int x = 0; x < Console::W; x++) {
        float cameraX = 2.0f * x / (float)Console::W - 1.0f;
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;
        
        int mapX = (int)player_x;
        int mapY = (int)player_y;
        
        float sideDistX, sideDistY;
        float deltaDistX = (rayDirX == 0) ? 1e30f : std::abs(1.0f / rayDirX);
        float deltaDistY = (rayDirY == 0) ? 1e30f : std::abs(1.0f / rayDirY);
        float perpWallDist;
        
        int stepX, stepY;
        int hit = 0;
        int side = 0;
        
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (player_x - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0f - player_x) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (player_y - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0f - player_y) * deltaDistY;
        }
        
        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }
            if (mapX >= 0 && mapX < MAP_WIDTH && mapY >= 0 && mapY < MAP_HEIGHT) {
                if (map[mapY][mapX] > 0) hit = map[mapY][mapX];
            } else {
                hit = 1;
                break;
            }
        }
        
        if (side == 0) perpWallDist = (sideDistX - deltaDistX);
        else           perpWallDist = (sideDistY - deltaDistY);
        
        zBuffer[x] = perpWallDist;
        
        int lineHeight = static_cast<int>(Console::H / (perpWallDist + 0.0001f));
        int drawStart = -lineHeight / 2 + Console::H / 2 + viewBobY + shakeY;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + Console::H / 2 + viewBobY + shakeY;
        if (drawEnd >= Console::H) drawEnd = Console::H - 1;
        
        int drawX = x + shakeX;
        if (drawX < 0 || drawX >= Console::W) continue;

        ctx.setDrawColor(Console::COLOR_WHITE);
        if (hit == 9) { 
            if (x % 2 == 0) ctx.drawVLine(drawX, drawStart, drawEnd - drawStart + 1);
        } else {
            uint8_t shade = (side == 0) ? 4 : 2;
            if (perpWallDist > 8.0f) shade = (side == 0) ? 2 : 1;
            
            ctx.drawDitherBox(drawX, drawStart, 1, drawEnd - drawStart + 1, shade);
            
            ctx.setDrawColor(Console::COLOR_BLACK);
            if (x > 0 && (mapX != lastMapX || mapY != lastMapY)) {
                ctx.drawVLine(drawX, drawStart, drawEnd - drawStart + 1);
            }
            ctx.drawPixel(drawX, drawStart);
            ctx.drawPixel(drawX, drawEnd);
        }
        
        lastMapX = mapX;
        lastMapY = mapY;
    }
    
    // SPRITE RENDERING
    for (int i = 0; i < num_sprites; i++) {
        sprites[i].distance = ((player_x - sprites[i].x) * (player_x - sprites[i].x) + (player_y - sprites[i].y) * (player_y - sprites[i].y));
    }
    for (int i = 0; i < num_sprites - 1; i++) {
        for (int j = 0; j < num_sprites - i - 1; j++) {
            if (sprites[j].distance < sprites[j+1].distance) {
                DoomSprite temp = sprites[j];
                sprites[j] = sprites[j+1];
                sprites[j+1] = temp;
            }
        }
    }
    
    for (int i = 0; i < num_sprites; i++) {
        if (!sprites[i].active) continue;
        
        float spriteX = sprites[i].x - player_x;
        float spriteY = sprites[i].y - player_y;
        
        float invDet = 1.0f / (planeX * dirY - dirX * planeY);
        float transformX = invDet * (dirY * spriteX - dirX * spriteY);
        float transformY = invDet * (-planeY * spriteX + planeX * spriteY);
        
        if (transformY <= 0) continue; 
        
        int spriteScreenX = static_cast<int>((Console::W / 2) * (1 + transformX / transformY));
        int spriteHeight = std::abs(static_cast<int>(Console::H / transformY));
        int drawStartY = -spriteHeight / 2 + Console::H / 2 + viewBobY + shakeY;
        int drawEndY = spriteHeight / 2 + Console::H / 2 + viewBobY + shakeY;
        
        int spriteWidth = std::abs(static_cast<int>(Console::H / transformY));
        int drawStartX = -spriteWidth / 2 + spriteScreenX;
        int drawEndX = spriteWidth / 2 + spriteScreenX;
        
        const uint8_t* tex = (sprites[i].type == 1) ? tex_skull : tex_medkit;
        const uint8_t* tex_mask = (sprites[i].type == 1) ? tex_skull_mask : tex_medkit_mask;
        
        for (int stripe = drawStartX; stripe < drawEndX; stripe++) {
            if (stripe < 0 || stripe >= Console::W) continue; // FIX out of bounds access to zBuffer
            
            int texX = static_cast<int>(256 * (stripe - (-spriteWidth / 2 + spriteScreenX)) * 8 / spriteWidth) / 256;
            int drawX = stripe + shakeX;
            
            if (transformY > 0 && drawX >= 0 && drawX < Console::W && transformY < zBuffer[stripe]) {
                for (int y = drawStartY; y < drawEndY; y++) {
                    int d = y * 256 - Console::H * 128 - viewBobY * 256 - shakeY * 256 + spriteHeight * 128;
                    int texY = ((d * 8) / spriteHeight) / 256;
                    if (texX >= 0 && texX < 8 && texY >= 0 && texY < 8) {
                        if (tex_mask[texY] & (1 << (7 - texX))) {
                            ctx.setDrawColor(Console::COLOR_BLACK);
                            ctx.drawPixel(drawX, y);
                        }
                        if (tex[texY] & (1 << (7 - texX))) {
                            ctx.setDrawColor(Console::COLOR_WHITE);
                            ctx.drawPixel(drawX, y);
                        }
                    }
                }
            }
        }
    }
    
    _particles->draw(ctx);
    
    // Draw Gun HUD 
    int bobY = static_cast<int>(std::sin(weapon_bob) * 3.0f);
    int gunX = Console::W / 2;
    int gunY = Console::H - 12 + bobY;
    if (fire_timer > 5) gunY += 5; 
    
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawBox(gunX - 5, gunY - 1, 10, Console::H - gunY + 2); 
    ctx.drawBox(gunX - 3, gunY - 5, 6, 6);
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.drawBox(gunX - 4, gunY, 8, Console::H - gunY); 
    ctx.drawBox(gunX - 2, gunY - 4, 4, 4); 
    
    if (fire_timer > 7) {
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawCircle(gunX, gunY - 6, 6);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawCircle(gunX, gunY - 6, 4);
    }
    
    // Crosshair
    if (fire_timer == 0) {
        ctx.setDrawColor(Console::COLOR_XOR);
        ctx.drawPixel(Console::W / 2 - 2, Console::H / 2);
        ctx.drawPixel(Console::W / 2 + 2, Console::H / 2);
        ctx.drawPixel(Console::W / 2, Console::H / 2 - 2);
        ctx.drawPixel(Console::W / 2, Console::H / 2 + 2);
        ctx.drawPixel(Console::W / 2,     Console::H / 2);
    }
    
    // HUD Stats Bar at Top
    ctx.setDrawColor(Console::COLOR_BLACK);
    ctx.drawBox(0, 0, Console::W, 9);
    ctx.setDrawColor(Console::COLOR_WHITE);
    ctx.drawHLine(0, 8, Console::W);
    ctx.setFont(u8g2_font_4x6_tf);
    ctx.drawPrintf(2, 6, "SC:%d HP:%d AMMO:%d", _data->score, _data->hp, _data->ammo);
    
    // Minimap
    if (show_minimap) {
        int mm_size = MAP_WIDTH * 2;
        int mm_x = Console::W - mm_size - 4;
        int mm_y = Console::H - mm_size - 4; // Bottom right corner
        ctx.setDrawColor(Console::COLOR_BLACK);
        ctx.drawBox(mm_x - 2, mm_y - 2, mm_size + 4, mm_size + 4);
        ctx.setDrawColor(Console::COLOR_WHITE);
        ctx.drawFrame(mm_x - 2, mm_y - 2, mm_size + 4, mm_size + 4);
        
        for (int my = 0; my < MAP_HEIGHT; my++) {
            for (int mx = 0; mx < MAP_WIDTH; mx++) {
                if (map[my][mx] > 0 && map[my][mx] < 5) {
                    ctx.drawBox(mm_x + mx * 2, mm_y + my * 2, 2, 2);
                } else if (map[my][mx] == 9) {
                    if (frame_counter % 10 < 5) ctx.drawBox(mm_x + mx * 2, mm_y + my * 2, 2, 2);
                }
            }
        }
        if (frame_counter % 4 < 2) {
            ctx.drawPixel(mm_x + (int)(player_x * 2), mm_y + (int)(player_y * 2));
        }
    }
}

// =============================================================================
// DoomGame Boilerplate
// =============================================================================

void DoomGame::onEnter(Console& ctx) {
    this->_data.hiScore = ctx.loadHiScore();
    _title.setShared(&this->_data);
    _play.setShared(&this->_data);
    _win.setShared(&this->_data);
    _play.setEngine(&this->_camera, &this->_particles);

    this->useDefaultEvents(nullptr, nullptr);
    this->_sm.onEvent(Event::CUSTOM_1, SceneManager::REPLACE, &_play);
    this->_sm.onEvent(Event::CUSTOM_2, SceneManager::REPLACE, &_title);
    this->_sm.onEvent(Event::CUSTOM_3, SceneManager::REPLACE, &_win);
    this->_sm.replace(&_title, ctx);
}

const char* DoomGame::getName() const {
    return "Doom";
}

REGISTER_GAME(DoomGame);
