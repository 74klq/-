#pragma once
#include <vector>
#include "raylib.h"
#include "../editing/chart_save.h"

struct HoldNoteObject {
    int lane;
    float startTimeSec;
    float endTimeSec;
    bool active;
    bool headHit;
    bool isHolding;
    bool completed;
    float tickAccumulator;
};

class HoldNoteManager {
public:
    static void Init();
    static void Clear();
    static void LoadFromChart(const std::vector<SaveNoteData>& notes);
    static void Update(float songTimer, float noteScrollSpeed, float judgmentLineY, 
                       int& combo, int& lastCombo, int& score, float& hpRatio, 
                       int& perfectCount, int& greatCount, int& goodCount, int& missCount,
                       bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer);
    static void Draw(float songTimer, float noteScrollSpeed, float judgmentLineY);

private:
    static std::vector<HoldNoteObject> s_HoldNotes;
};