#pragma once
#include <raylib.h>

class Shack
{
public:
    static void Trigger(float duration = 0.1f, float magnitude = 6.0f);
    static void Update();
    static void Begin();
    static void End();
};