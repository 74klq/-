// 참고로 유령코드입니다 (지우기 귀찮아서 방치중)

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