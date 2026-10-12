#include "shack.h"
#include <cstdlib>

static float s_ShackTimer = 0.0f;
static float s_ShackDuration = 0.1f;
static float s_ShackMagnitude = 6.0f;
static Camera2D s_ShackCamera = { 0 };

void Shack::Trigger(float duration, float magnitude)
{
    s_ShackDuration = duration;
    s_ShackTimer = duration;
    s_ShackMagnitude = magnitude;
}

void Shack::Update()
{
    if (s_ShackTimer > 0.0f)
    {
        s_ShackTimer -= GetFrameTime();
        if (s_ShackTimer < 0.0f)
        {
            s_ShackTimer = 0.0f;
        }
    }
}

void Shack::Begin()
{
    s_ShackCamera = { 0 };
    s_ShackCamera.zoom = 1.0f;
    if (s_ShackTimer > 0.0f)
    {
        float progress = s_ShackTimer / s_ShackDuration;
        float currentMag = s_ShackMagnitude * progress;
        float offsetX = ((float)(rand() % 2000) / 1000.0f - 1.0f) * currentMag;
        float offsetY = ((float)(rand() % 2000) / 1000.0f - 1.0f) * currentMag;
        s_ShackCamera.offset = (Vector2){ offsetX, offsetY };
    }
    BeginMode2D(s_ShackCamera);
}

void Shack::End()
{
    EndMode2D();
}