#include "Camera.h"
#include <Arduino.h>

void Camera::snapTo(int newX, int newY) {
    x = _startX = _targetX = newX;
    y = _startY = _targetY = newY;
    _t = 0;
    _totalFrames = 0;
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

void Camera::shake(uint8_t frames) {
    _shakeFrames = frames;
}

void Camera::update() {
    if (_t < _totalFrames) {
        _t++;
        x = lerpi(_startX, _targetX, _t, _totalFrames);
        y = lerpi(_startY, _targetY, _t, _totalFrames);
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
