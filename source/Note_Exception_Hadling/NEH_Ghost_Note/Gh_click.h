#ifndef GH_CLICK_H
#define GH_CLICK_H

#include <vector>

struct PlayableNote;

class GhClick
{
public:
    static bool IsLaneHitTargetAvailable(int lane, float songTimerSec, const std::vector<PlayableNote>& notes, float hitWindowSec = 0.15f);
    static void TriggerBreak(int& combo, int& lastCombo, bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer);
};

#endif