#include "Fn_click.h"
#include <cmath>

struct PlayableNote {
    float timeSec;
    int lane;
    int type;
    bool active;
};

bool FnClick::IsFastHit(int lane, float songTimerSec, const std::vector<PlayableNote>& notes, float hitWindowSec, float fastWindowSec)
{
    for (const auto& note : notes)
    {
        if (!note.active || note.lane != lane)
            continue;

        float timeUntilNote = note.timeSec - songTimerSec;

        if (timeUntilNote > hitWindowSec && timeUntilNote <= fastWindowSec)
        {
            return true;
        }
    }
    return false;
}

void FnClick::TriggerFast(int& combo, int& lastCombo, bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer)
{
    combo = 0;
    lastCombo = 0;
    showJudgment = true;
    judgmentTimer = 0.4f;
    currentJudgment = "FAST";
    judgmentAnimTimer = 0.3f;
}