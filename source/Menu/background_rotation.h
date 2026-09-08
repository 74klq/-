#ifndef BACKGROUND_ROTATION_H
#define BACKGROUND_ROTATION_H

#include "raylib.h"
#include <vector>
#include <string>

class BackgroundRotation {
private:
    std::vector<Texture2D> backgrounds;
    int currentIndex;
    int nextIndex;
    float timer;
    float switchInterval;
    float transitionProgress;
    bool isTransitioning;
    float idleAnimTime;

public:
    BackgroundRotation();
    ~BackgroundRotation();

    void Update();
    void Draw(int screenWidth, int screenHeight);

    int GetCurrentIndex() const { return currentIndex; }
    int GetNextIndex() const { return nextIndex; }
    float GetTransitionProgress() const { return transitionProgress; }
    bool IsTransitioning() const { return isTransitioning; }
};

#endif