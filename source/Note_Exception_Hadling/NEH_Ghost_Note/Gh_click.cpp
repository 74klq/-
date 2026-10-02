#include "Gh_click.h"
#include <cmath>

struct PlayableNote {
    float timeSec;
    int lane;
    int type;
    bool active;
};

bool GhClick::IsLaneHitTargetAvailable(int lane, float songTimerSec, const std::vector<PlayableNote>& notes, float hitWindowSec)
{
    for (const auto& note : notes)
    {
        if (!note.active || note.lane != lane)
            continue;

        float timeDiff = std::fabs(songTimerSec - note.timeSec);
        if (timeDiff <= hitWindowSec)
        {
            return true;
        }
    }
    return false;
}

void GhClick::TriggerBreak(int& combo, int& lastCombo, bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer)
{
    combo = 0;
    lastCombo = 0;
    showJudgment = true;
    judgmentTimer = 0.4f;
    currentJudgment = "BREAK";
    judgmentAnimTimer = 0.3f;
}