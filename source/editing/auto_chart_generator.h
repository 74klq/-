#pragma once
#include <vector>
#include <fmod.h>
#include "chart_save.h"

class AutoChartGenerator {
public:
    AutoChartGenerator();
    ~AutoChartGenerator();

    void Init();
    std::vector<SaveNoteData> GenerateNotesFromFFT(FMOD_DSP* fftDsp, float currentSec, float scrollSpeed);

private:
    float m_CooldownTimer;
    float m_PrevLowEnergy;
    float m_PrevMidEnergy;
    float m_PrevHighEnergy;
    float m_RunningFluxAvg;
};