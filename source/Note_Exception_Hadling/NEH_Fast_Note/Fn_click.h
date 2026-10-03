#ifndef FN_CLICK_H
#define FN_CLICK_H

#include <vector>

struct PlayableNote;

class FnClick
{
public:
    static bool IsFastHit(int lane, float songTimerSec, const std::vector<PlayableNote>& notes, float hitWindowSec = 0.15f, float fastWindowSec = 0.35f);
    static void TriggerFast(int& combo, int& lastCombo, bool& showJudgment, float& judgmentTimer, const char*& currentJudgment, float& judgmentAnimTimer);
};

#endif