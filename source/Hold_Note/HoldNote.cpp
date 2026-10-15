#include "HoldNote.h"
#include <cmath>
#include <algorithm>

std::vector<HoldNoteObject> HoldNoteManager::s_HoldNotes;

static inline float GetHoldLaneXCoord(int lane) {
    extern float GetLaneXCoord(int lane);
    return GetLaneXCoord(lane);
}

void HoldNoteManager::Init() {
    s_HoldNotes.clear();
}

void HoldNoteManager::Clear() {
    s_HoldNotes.clear();
}

void HoldNoteManager::LoadFromChart(const std::vector<SaveNoteData>& notes) {
    s_HoldNotes.clear();
    for (const auto& noteData : notes) {
        if ((noteData.type & 128) != 0) {
            HoldNoteObject obj;
            obj.lane = noteData.lane;
            obj.startTimeSec = static_cast<float>(noteData.time) / 1000.0f;
            obj.endTimeSec = static_cast<float>(noteData.endTime) / 1000.0f;
            obj.active = true;
            obj.headHit = false;
            obj.isHolding = false;
            obj.completed = false;
            obj.tickAccumulator = 0.0f;
            s_HoldNotes.push_back(obj);
        }
    }
}

void HoldNoteManager::Update(float songTimer, float noteScrollSpeed, float judgmentLineY,
                             int& combo, int& lastCombo, int& score, float& hpRatio,
                             int& perfectCount, int& greatCount, int& goodCount, int& missCount,
                             bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer) {
    float dt = GetFrameTime();

    bool keyPressStates[4] = { IsKeyPressed(KEY_D), IsKeyPressed(KEY_F), IsKeyPressed(KEY_J), IsKeyPressed(KEY_K) };
    bool keyDownStates[4]  = { IsKeyDown(KEY_D),  IsKeyDown(KEY_F),  IsKeyDown(KEY_J),  IsKeyDown(KEY_K)  };
    bool keyPressUsed[4] = { false, false, false, false };

    for (auto& note : s_HoldNotes) {
        if (!note.active) continue;
        if (note.lane < 0 || note.lane >= 4) continue;

        if (!note.headHit) {
            float timeDiff = songTimer - note.startTimeSec;
            
            if (timeDiff > 0.45f) {
                note.active = false;
                note.completed = true;
                combo = 0;
                lastCombo = 0;
                showJudgment = true;
                judgmentTimer = 0.3f;
                currentJudgment = "MISS";
                judgmentAnimTimer = 0.3f;
                missCount++;
                hpRatio -= 0.05f;
                if (hpRatio < 0.0f) hpRatio = 0.0f;
                continue;
            }

            if (keyPressUsed[note.lane]) continue;

            if (keyPressStates[note.lane]) {
                float absDiff = fabsf(timeDiff);
                if (absDiff <= 0.22f) {
                    note.headHit = true;
                    keyPressUsed[note.lane] = true;

                    if (keyDownStates[note.lane]) {
                        note.isHolding = true;
                    }

                    if (absDiff <= 0.12f) {
                        currentJudgment = "PERFECT";
                        combo++;
                        perfectCount++;
                        score += 20;
                        hpRatio += 0.01f;
                    } else if (absDiff <= 0.18f) {
                        currentJudgment = "GREAT";
                        combo++;
                        greatCount++;
                        score += 10;
                        hpRatio += 0.005f;
                    } else {
                        currentJudgment = "GOOD";
                        combo++;
                        goodCount++;
                        score += 5;
                    }
                    if (hpRatio > 1.0f) hpRatio = 1.0f;
                    showJudgment = true;
                    judgmentTimer = 0.4f;
                    judgmentAnimTimer = 0.3f;
                }
            }
        } 
        else if (note.isHolding) {
            if (songTimer >= note.endTimeSec - 0.05f) { 
                note.isHolding = false;
                note.active = false;
                note.completed = true;
                combo++;
                score += 15;
                hpRatio += 0.01f;
                if (hpRatio > 1.0f) hpRatio = 1.0f;
                currentJudgment = "PERFECT";
                showJudgment = true;
                judgmentTimer = 0.4f;
                judgmentAnimTimer = 0.3f;
            }
            else if (!keyDownStates[note.lane]) {
                float remainingTime = note.endTimeSec - songTimer;
                
                if (remainingTime < 0.18f) {
                    note.isHolding = false;
                    note.active = false;
                    note.completed = true;
                    combo++;
                    score += 15;
                    hpRatio += 0.01f;
                    if (hpRatio > 1.0f) hpRatio = 1.0f;
                    currentJudgment = "PERFECT";
                    showJudgment = true;
                    judgmentTimer = 0.4f;
                    judgmentAnimTimer = 0.3f;
                } else {
                    note.isHolding = false;
                    note.active = false;
                    note.completed = true;
                    combo = 0;
                    lastCombo = 0;
                    showJudgment = true;
                    judgmentTimer = 0.3f;
                    currentJudgment = "MISS";
                    judgmentAnimTimer = 0.3f;
                    missCount++;
                    hpRatio -= 0.05f;
                    if (hpRatio < 0.0f) hpRatio = 0.0f;
                }
            }
            else {
                note.tickAccumulator += dt;
                while (note.tickAccumulator >= 0.1f) {
                    note.tickAccumulator -= 0.1f;
                    combo++;
                    score += 2;
                }
            }
        }
    }
}

void HoldNoteManager::Draw(float songTimer, float noteScrollSpeed, float judgmentLineY) {
    float nowMs = songTimer * 1000.0f;

    for (const auto& note : s_HoldNotes) {
        if (!note.active && note.completed) continue;
        if (note.lane < 0 || note.lane >= 4) continue;

        float startDiffSec = (note.startTimeSec * 1000.0f - nowMs) / 1000.0f;
        float endDiffSec = (note.endTimeSec * 1000.0f - nowMs) / 1000.0f;

        float headY = judgmentLineY - (startDiffSec * noteScrollSpeed);
        float tailY = judgmentLineY - (endDiffSec * noteScrollSpeed);

        if (note.headHit && note.isHolding) {
            headY = judgmentLineY;
        }

        if (tailY < -500.0f || headY > (float)GetScreenHeight() + 500.0f) continue;

        float cx = GetHoldLaneXCoord(note.lane);
        float w = (400.0f / 4.0f) - 6.0f;
        float bodyHeight = headY - tailY;

        if (bodyHeight < 0.0f) bodyHeight = 0.0f;

        DrawRectangleRounded({ cx - w * 0.5f, tailY, w, bodyHeight }, 0.1f, 4, Color{ 255, 255, 255, 120 });
        DrawRectangleRounded({ cx - w * 0.5f, headY - 12.0f, w, 24.0f }, 0.2f, 6, Color{ 218, 218, 221, 255 });
    }
}
