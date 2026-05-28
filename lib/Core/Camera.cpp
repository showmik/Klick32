#include "Camera.h"
#include <Arduino.h>

void Camera::snapTo(int newX, int newY) {
    x = _startX = _targetX = newX;
    y = _startY = _targetY = newY;
    _t = 0;
    _totalFrames = 0;
    _applyBounds();
}

void Camera::panTo(int newX, int newY, int durationFrames) {
    if (durationFrames <= 0) {
        snapTo(newX, newY);
        return;
    }
    
    // Only restart pan if the target changed
    if (newX != _targetX || newY != _targetY) {
        _startX = x;
        _startY = y;
        _targetX = newX;
        _targetY = newY;
        _t = 0;
        _totalFrames = durationFrames;
    }
}

void Camera::follow(int targetX, int targetY, float lerpFactor) {
    // Disable any active pan
    _totalFrames = 0;
    _t = 0;
    
    x = (int)lerpf((float)x, (float)targetX, lerpFactor);
    y = (int)lerpf((float)y, (float)targetY, lerpFactor);
    _applyBounds();
}

void Camera::setBounds(int minX, int minY, int maxX, int maxY) {
    _hasBounds = true;
    _minX = minX;
    _minY = minY;
    _maxX = maxX;
    _maxY = maxY;
    _applyBounds();
}

void Camera::clearBounds() {
    _hasBounds = false;
}

void Camera::_applyBounds() {
    if (_hasBounds) {
        x = gclamp(x, _minX, _maxX);
        y = gclamp(y, _minY, _maxY);
    }
}

void Camera::shake(uint8_t frames) {
    _shakeFrames = frames;
}

void Camera::update() {
    if (_t < _totalFrames) {
        _t++;
        x = lerpi(_startX, _targetX, _t, _totalFrames);
        y = lerpi(_startY, _targetY, _t, _totalFrames);
        _applyBounds();
    }
    
    if (_shakeFrames > 0) {
        _shakeFrames--;
        _shakeOffsetX = random(-2, 3);
        _shakeOffsetY = random(-2, 3);
    } else {
        _shakeOffsetX = 0;
        _shakeOffsetY = 0;
    }
}

int Camera::getOffsetX() const {
    return -x + _shakeOffsetX;
}

int Camera::getOffsetY() const {
    return -y + _shakeOffsetY;
}
