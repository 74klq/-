#include "auto_chart_generator.h"
#include <fmod_dsp.h>
#include <algorithm>
#include <cstdlib>

AutoChartGenerator::AutoChartGenerator() 
    : m_CooldownTimer(0.0f), 
      m_PrevLowEnergy(0.0f), 
      m_PrevMidEnergy(0.0f), 
      m_PrevHighEnergy(0.0f),
      m_RunningFluxAvg(0.0f) {}

AutoChartGenerator::~AutoChartGenerator() {}

void AutoChartGenerator::Init() {
    m_CooldownTimer = 0.0f;
    m_PrevLowEnergy = 0.0f;
    m_PrevMidEnergy = 0.0f;
    m_PrevHighEnergy = 0.0f;
    m_RunningFluxAvg = 0.0f;
}

std::vector<SaveNoteData> AutoChartGenerator::GenerateNotesFromFFT(FMOD_DSP* fftDsp, float currentSec, float scrollSpeed) {
    std::vector<SaveNoteData> newNotes;
    if (!fftDsp) return newNotes;

    if (m_CooldownTimer > 0.0f) {
        m_CooldownTimer -= 0.016f;
        return newNotes;
    }

    void* fftData = nullptr;
    unsigned int dataLen = 0;

    if (FMOD_DSP_GetParameterData(fftDsp, FMOD_DSP_FFT_SPECTRUMDATA, &fftData, &dataLen, nullptr, 0) == FMOD_OK && fftData != nullptr) {
        FMOD_DSP_PARAMETER_FFT* fft = (FMOD_DSP_PARAMETER_FFT*)fftData;

        if (fft->numchannels > 0 && fft->length >= 32) {
            float lowEnergy = 0.0f;
            float midEnergy = 0.0f;
            float highEnergy = 0.0f;

            for (int i = 0; i < 8; ++i) {
                lowEnergy += fft->spectrum[0][i];
            }
            for (int i = 8; i < 22; ++i) {
                midEnergy += fft->spectrum[0][i];
            }
            for (int i = 22; i < 32; ++i) {
                highEnergy += fft->spectrum[0][i];
            }

            // Calculate positive energy flux (half-wave rectified frame-to-frame rate of change)
            // Gradual volume swells, fades, and natural decays produce low continuous flux and are naturally filtered out.
            float lowFlux = std::max(0.0f, lowEnergy - m_PrevLowEnergy);
            float midFlux = std::max(0.0f, midEnergy - m_PrevMidEnergy);
            float highFlux = std::max(0.0f, highEnergy - m_PrevHighEnergy);

            float totalFlux = lowFlux * 1.2f + midFlux * 0.8f + highFlux * 0.2f;

            // Maintain a running average of the flux to establish a baseline rate of change
            if (m_RunningFluxAvg == 0.0f) {
                m_RunningFluxAvg = totalFlux;
            } else {
                m_RunningFluxAvg = m_RunningFluxAvg * 0.92f + totalFlux * 0.08f;
            }

            // Detect sharp instantaneous transient attacks (onsets) that abruptly break the local average rate of change
            bool isTransientAttack = (totalFlux > (m_RunningFluxAvg * 3.0f + 0.015f)) && (lowFlux > 0.008f || midFlux > 0.012f);

            if (isTransientAttack) {
                int assignedLane = rand() % 4;
                float posX = currentSec * scrollSpeed;

                newNotes.push_back({ assignedLane, static_cast<int>(posX) });
                
                // Strict refractory cooldown period to guarantee one note per rhythmic attack
                m_CooldownTimer = 0.25f;
            }

            m_PrevLowEnergy = lowEnergy;
            m_PrevMidEnergy = midEnergy;
            m_PrevHighEnergy = highEnergy;
        }
    }

    return newNotes;
}