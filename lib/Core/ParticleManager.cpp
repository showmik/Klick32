#include "ParticleManager.h"

void ParticleManager::spawnPixel(float x, float y, float vx, float vy, uint8_t life) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!_particles[i].active) {
            _particles[i].x = x;
            _particles[i].y = y;
            _particles[i].vx = vx;
            _particles[i].vy = vy;
            _particles[i].life = life;
            _particles[i].startLife = life;
            _particles[i].active = true;
            break;
        }
    }
}

void ParticleManager::update() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (_particles[i].active) {
            _particles[i].x += _particles[i].vx;
            _particles[i].y += _particles[i].vy;
            
            if (_particles[i].life > 0) {
                _particles[i].life--;
            } else {
                _particles[i].active = false;
            }
        }
    }
}

void ParticleManager::draw(Console& ctx) const {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (_particles[i].active) {
            // Simple pixel draw. A more complex system might draw bitmaps based on a type enum.
            ctx.drawPixel((int)_particles[i].x, (int)_particles[i].y);
        }
    }
}

void ParticleManager::clear() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].active = false;
    }
}
