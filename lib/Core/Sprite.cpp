#include "Sprite.h"

Sprite::Sprite(float startX, float startY, int w, int h, const uint8_t* bmp, int bpr)
    : x(startX), y(startY), width(w), height(h), bitmap(bmp), bytesPerRow(bpr), active(true)
{
}

void Sprite::draw(Console& ctx) const {
    if (!active || !bitmap) return;
    ctx.drawBitmap((int)x, (int)y, bytesPerRow, height, bitmap);
}

bool Sprite::collidesWith(const Sprite& other) const {
    if (!active || !other.active) return false;

    // AABB intersection test
    return x < other.x + other.width &&
           x + width > other.x &&
           y < other.y + other.height &&
           y + height > other.y;
}

bool Sprite::contains(float px, float py) const {
    if (!active) return false;
    return px >= x && px < x + width && py >= y && py < y + height;
}
