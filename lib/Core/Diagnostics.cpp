#include "Diagnostics.h"

bool Diagnostics::_visible = false;
uint32_t Diagnostics::_lastTime = 0;
uint32_t Diagnostics::_frameCount = 0;
uint16_t Diagnostics::_fps = 0;
uint32_t Diagnostics::_updateStart = 0;
uint32_t Diagnostics::_updateEnd = 0;
Diagnostics::DebugRect Diagnostics::_rects[MAX_DEBUG_RECTS];
int Diagnostics::_rectCount = 0;
