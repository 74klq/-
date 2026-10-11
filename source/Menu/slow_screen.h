#ifndef SLOW_SCREEN_H
#define SLOW_SCREEN_H

#include <raylib.h>

class SlowScreen {
public:
    static void Init();
    static void Update(float dt);
    static void Draw(int screenWidth, int screenHeight);
    static bool IsFinished();

private:
    static float fadeAlpha;
    static float timer;
    static bool initialized;
};

#endif