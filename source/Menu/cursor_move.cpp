#include "cursor_move.h"
#include <cmath>

ParallaxTransform CursorMove::GetTransform(float depth, float intensity) {
    Vector2 mouse = GetMousePosition();
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    float centerX = screenW * 0.5f;
    float centerY = screenH * 0.5f;

    float dx = (mouse.x - centerX) / centerX;
    float dy = (mouse.y - centerY) / centerY;

    float offsetX = -dx * screenW * 0.03f * intensity * depth;
    float offsetY = -dy * screenH * 0.03f * intensity * depth;
    float rotation = -dx * 1.8f * depth;
    float scale = 1.0f + std::abs(dx) * 0.015f * depth + std::abs(dy) * 0.015f * depth;

    return { { offsetX, offsetY }, scale, rotation };
}