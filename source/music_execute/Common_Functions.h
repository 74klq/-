#pragma once
#include "../AudioManager/audio_manager.h"
#include <fmod.h>
#include <raylib.h>

namespace MusicExecuteUtils {
    inline void HandleMusicControls(FMOD_CHANNEL* channel, bool& isPaused, float& outSpeedRatio, float& outMsgTimer, float& inputCooldownTimer) {
        // 입력 쿨다운(잔상 방지 타이머)이 남아있다면 키 입력을 완전히 무시합니다.
        if (inputCooldownTimer > 0.0f) {
            inputCooldownTimer -= GetFrameTime();
            // 쿨다운 중에는 입력 처리를 건너뜁니다.
            return;
        }

        if (IsKeyPressed(KEY_G)) {
            if (channel) {
                FMOD_Channel_SetPaused(channel, 0);
                isPaused = false;
            }
        }

        if (IsKeyPressed(KEY_R)) {
            if (channel) {
                FMOD_Channel_SetPaused(channel, 1);
                isPaused = true;
            }
        }

        if (!channel) return;

        float currentFrequency = 0.0f;
        FMOD_SOUND* currentSound = nullptr;
        float originalFrequency = 44100.0f;

        FMOD_Channel_GetCurrentSound(channel, &currentSound);
        if (currentSound) {
            int priority = 0;
            FMOD_Sound_GetDefaults(currentSound, &originalFrequency, &priority);
        }

        FMOD_Channel_GetFrequency(channel, &currentFrequency);
        if (originalFrequency <= 0.0f) {
            originalFrequency = 44100.0f; 
        }

        float currentSpeedRatio = currentFrequency / originalFrequency;
        float step = 0.05f;
        bool speedChanged = false;

        if (IsKeyPressed(KEY_L)) {
            float newRatio = currentSpeedRatio - step;
            if (newRatio < 0.25f) newRatio = 0.25f;
            FMOD_Channel_SetFrequency(channel, originalFrequency * newRatio);
            currentSpeedRatio = newRatio;
            speedChanged = true;
        }

        if (IsKeyPressed(KEY_X)) {
            float newRatio = currentSpeedRatio + step;
            if (newRatio > 1.0f) {
                newRatio = 1.0f;
            }
            FMOD_Channel_SetFrequency(channel, originalFrequency * newRatio);
            currentSpeedRatio = newRatio;
            speedChanged = true;
        }

        outSpeedRatio = currentSpeedRatio;
        if (speedChanged) {
            outMsgTimer = 3.0f;
        }
    }
}