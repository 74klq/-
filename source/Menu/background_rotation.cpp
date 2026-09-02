#include "background_rotation.h"
#include <cstdlib>
#include <ctime>
#include <cmath>

BackgroundRotation::BackgroundRotation() {
    currentIndex = 0;
    nextIndex = 0;
    timer = 0.0f;
    switchInterval = 4.0f;
    transitionProgress = 0.0f;
    isTransitioning = false;
    idleAnimTime = 0.0f;

    srand((unsigned int)time(nullptr));

    for (int i = 1; i <= 6; ++i) {
        std::string path = "assets/background" + std::to_string(i) + ".png";
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
            backgrounds.push_back(tex);
        }
    }

    if (!backgrounds.empty()) {
        currentIndex = rand() % backgrounds.size();
        nextIndex = currentIndex;
    }
}

BackgroundRotation::~BackgroundRotation() {
    for (auto& tex : backgrounds) {
        UnloadTexture(tex);
    }
    backgrounds.clear();
}

void BackgroundRotation::Update() {
    if (backgrounds.size() <= 1) return;

    float dt = GetFrameTime();
    timer += dt;
    idleAnimTime += dt;

    if (!isTransitioning) {
        if (timer >= switchInterval) {
            timer = 0.0f;
            int newIdx;
            do {
                newIdx = rand() % backgrounds.size();
            } while (newIdx == currentIndex);
            
            nextIndex = newIdx;
            isTransitioning = true;
            transitionProgress = 0.0f;
        }
    } else {
        transitionProgress += dt * 0.35f;
        if (transitionProgress >= 1.0f) {
            transitionProgress = 1.0f;
            currentIndex = nextIndex;
            isTransitioning = false;
        }
    }
}

void BackgroundRotation::Draw(int screenWidth, int screenHeight) {
    if (backgrounds.empty()) return;

    float globalZoom = 1.05f + (std::sin(idleAnimTime * 0.4f) * 0.03f);
    float idleOffsetX = std::cos(idleAnimTime * 0.3f) * 12.0f;
    float idleOffsetY = std::sin(idleAnimTime * 0.35f) * 8.0f;

    float w = (float)screenWidth * globalZoom;
    float h = (float)screenHeight * globalZoom;
    float x = ((float)screenWidth - w) / 2.0f + idleOffsetX;
    float y = ((float)screenHeight - h) / 2.0f + idleOffsetY;

    Texture2D currentTex = backgrounds[currentIndex];
    Rectangle sourceRec = { 0.0f, 0.0f, (float)currentTex.width, (float)currentTex.height };
    Vector2 origin = { 0.0f, 0.0f };
    Rectangle destRec = { x, y, w, h };

    if (!isTransitioning) {
        Color baseTint = { 255, 255, 255, 128 };
        DrawTexturePro(currentTex, sourceRec, destRec, origin, 0.0f, baseTint);
    } else {
        float t = transitionProgress;
        float easeT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

        unsigned char currentAlpha = (unsigned char)((1.0f - easeT) * 128.0f);
        DrawTexturePro(currentTex, sourceRec, destRec, origin, 0.0f, (Color){255, 255, 255, currentAlpha});

        Texture2D nextTex = backgrounds[nextIndex];
        unsigned char nextAlpha = (unsigned char)(easeT * 128.0f);
        DrawTexturePro(nextTex, sourceRec, destRec, origin, 0.0f, (Color){255, 255, 255, nextAlpha});
    }
}