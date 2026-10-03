#ifndef SN_CLICK_H
#define SN_CLICK_H

#include <vector>

struct PlayableNote;

class SnClick
{
public:
    static bool IsSlowHit(int lane, float songTimerSec, const std::vector<PlayableNote>& notes, float hitWindowSec = 0.15f, float slowWindowSec = 0.35f);
    static void TriggerSlow(int& combo, int& lastCombo, bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer);
};

#endif