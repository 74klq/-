#pragma once
#include "../AudioManager/audio_manager.h"
#include <fmod.h>
#include <raylib.h>
#include <string>

namespace MusicExecuteUtils {
    inline void HandleMusicControls(FMOD_CHANNEL* channel, bool& isPaused, float& outSpeedRatio, float& outMsgTimer, float& inputCooldownTimer, std::string& outMsgText) {
        if (inputCooldownTimer > 0.0f) {
            inputCooldownTimer -= GetFrameTime();
            return;
        }

        if (!channel) return;

        if (IsKeyPressed(KEY_SPACE)) {
            FMOD_BOOL paused = 0;
            FMOD_Channel_GetPaused(channel, &paused);
            FMOD_Channel_SetPaused(channel, !paused);
            isPaused = !paused;
            
            outMsgText = isPaused ? "Music Paused" : "Music Playing";
            outMsgTimer = 2.0f;
        }

        unsigned int currentPosMs = 0;
        FMOD_Channel_GetPosition(channel, &currentPosMs, FMOD_TIMEUNIT_MS);

        if (IsKeyPressed(KEY_G)) {
            int newPosMs = (int)currentPosMs - 5000;
            if (newPosMs < 0) newPosMs = 0;
            FMOD_Channel_SetPosition(channel, (unsigned int)newPosMs, FMOD_TIMEUNIT_MS);

            int sec = newPosMs / 1000;
            int min = sec / 60;
            sec %= 60;
            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "-5s (%02d:%02d)", min, sec);
            outMsgText = timeBuf;
            outMsgTimer = 2.0f;
        }

        if (IsKeyPressed(KEY_H)) {
            unsigned int newPosMs = currentPosMs + 5000;
            FMOD_Channel_SetPosition(channel, newPosMs, FMOD_TIMEUNIT_MS);

            int sec = newPosMs / 1000;
            int min = sec / 60;
            sec %= 60;
            char timeBuf[32];
            snprintf(timeBuf, sizeof(timeBuf), "+5s (%02d:%02d)", min, sec);
            outMsgText = timeBuf;
            outMsgTimer = 2.0f;
        }

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
            int percent = (int)(currentSpeedRatio * 100.0f + 0.5f);
            outMsgText = "Speed: " + std::to_string(percent) + "%";
            outMsgTimer = 2.0f;
        }
    }
}