#include "curtain_vfx.h"
#include <algorithm>

CurtainVFX::CurtainVFX() {
    currentState = IDLE;
    progress = 0.0f;
    speed = 2.5f;
    midPointCallback = nullptr;
}

void CurtainVFX::Init() {
    currentState = IDLE;
    progress = 0.0f;
    midPointCallback = nullptr;
}

void CurtainVFX::Start(std::function<void()> onMidPoint) {
    currentState = CLOSING;
    progress = 0.0f;
    midPointCallback = onMidPoint;
}

void CurtainVFX::Update(float dt) {
    if (currentState == IDLE) return;

    if (currentState == CLOSING) {
        progress += dt * speed;
        if (progress >= 1.0f) {
            progress = 1.0f;
            currentState = HOLD;
        }
    }
    else if (currentState == HOLD) {
        if (midPointCallback) {
            auto task = midPointCallback;
            midPointCallback = nullptr;
            task();
        }
        currentState = OPENING;
    }
    else if (currentState == OPENING) {
        progress -= dt * speed;
        if (progress <= 0.0f) {
            progress = 0.0f;
            currentState = IDLE;
            midPointCallback = nullptr;
        }
    }
}

void CurtainVFX::Draw(int screenWidth, int screenHeight) {
    if (currentState == IDLE) return;

    float halfWidth = (float)screenWidth * 0.5f;
    float currentWidth = halfWidth * progress;

    DrawRectangle(0, 0, (int)currentWidth, screenHeight, BLACK);
    DrawRectangle((int)(screenWidth - currentWidth), 0, (int)currentWidth, screenHeight, BLACK);
}

bool CurtainVFX::IsActive() const {
    return currentState != IDLE;
}
