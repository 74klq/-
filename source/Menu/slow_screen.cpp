#include "slow_screen.h"
#include <algorithm>

float SlowScreen::fadeAlpha = 1.0f;
float SlowScreen::timer = 0.0f;
bool SlowScreen::initialized = false;

void SlowScreen::Init() {
    fadeAlpha = 1.0f;
    timer = 0.0f;
    initialized = true;
}

void SlowScreen::Update(float dt) {
    if (!initialized) Init();

    timer += dt;
    float duration = 2.2f;
    if (timer < duration) {
        fadeAlpha = 1.0f - (timer / duration);
    } else {
        fadeAlpha = 0.0f;
    }
}

void SlowScreen::Draw(int screenWidth, int screenHeight) {
    if (!initialized) Init();

    if (fadeAlpha > 0.001f) {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, (unsigned char)(fadeAlpha * 255.0f) });
    }
}

bool SlowScreen::IsFinished() {
    return fadeAlpha <= 0.001f;
}