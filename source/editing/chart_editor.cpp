#include "chart_editor.h"
#include "chart_save.h"
#include "editor_play.h"

#include <fmod.h>
#include <fmod_dsp.h>

#include "../music_execute/music1_on.cpp"
#include "../AudioManager/audio_manager.h"
#include <cmath>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <random>

static std::vector<std::string> editorTexts = {
    "오디오를 눈에 보여주는거",
    "채보 에디터 프로그램",
    "'7'(수동) 또는 'A'(자동) 키를 눌러 레코딩",
    "현재 피치: ",
    "레코딩 중...- U 키로 정지",
    "'7': 실시간 에디팅 | 'A': 자동 에디팅 (AI) | '엔터': 테스트 | '0': 저장 | 'T': 차트 속도? | 'Q': 음악 선택",
    "속도 = ",
    "음악 선택 (화살표키랑 엔터):"
};

static EditorPlay s_EditorPlay;
static bool s_IsRecording = false;
static bool s_IsAutoRecording = false;
static bool s_IsTestPlaying = false;
static float s_CurrentPitch = 1.0f;
static float s_ScrollSpeed = 200.0f;
static bool s_IsSpeedInputActive = false;
static std::string s_SpeedInputString = "";

static float s_InputLatency = 0.040f; 
static float s_EditorBPM = 220.0f;
static float s_SongOffsetSec = 0.0f;
static float s_QuantizeThresholdMs = 45.0f; 

static const int ENERGY_HISTORY_SIZE = 20;
static float s_EnergyHistory[ENERGY_HISTORY_SIZE] = { 0.0f };
static int s_EnergyHistoryIndex = 0;
static int s_EnergyHistoryCount = 0;

static bool s_IsMusicSelectOpen = false;
static std::vector<std::string> s_MusicFileList;
static int s_SelectedMusicIndex = 0;
static std::string s_CurrentMusicPath = "";

static Font s_SuitFont = { 0 };
static bool s_IsFontLoaded = false;

static const float PLAYFIELD_X = 680.0f;
static const float PLAYFIELD_Y = 0.0f;
static const float PLAYFIELD_WIDTH = 320.0f;
static const float PLAYFIELD_HEIGHT = 720.0f;

static const int LANE_COUNT = 4;
static const float LANE_WIDTH = 70.0f;
static const float LANE_AREA_WIDTH = LANE_COUNT * LANE_WIDTH;
static const float LANE_START_X = PLAYFIELD_X + (PLAYFIELD_WIDTH - LANE_AREA_WIDTH) / 2.0f;

static const float LANE_X_COORDS[4] = {
    LANE_START_X + LANE_WIDTH * 0.5f,
    LANE_START_X + LANE_WIDTH * 1.5f,
    LANE_START_X + LANE_WIDTH * 2.5f,
    LANE_START_X + LANE_WIDTH * 3.5f
};

static const float JUDGMENT_LINE_Y = 595.0f;

struct EditorNoteData {
    int lane;
    float posX; 
    float snapErrorMs; 
    Color feedbackColor; 
};

static std::vector<EditorNoteData> s_EditorNotes; 

static float s_LanePulseTimers[6] = { 0.0f }; 
static float s_MetronomeFlashAlpha = 0.0f;  

static FMOD_DSP* s_FftDsp = nullptr;
static bool s_IsFftInitialized = false;

static float s_PrevEnergy = 0.0f;
static float s_LastAutoNoteTime = -1.0f;

static int s_PatternStep = 0;
static int s_LastAssignedLane = -1;
static int s_PatternDirection = 1;

static bool s_IsAudioStarted = false;

static int s_SameLaneCount = 0;
static int s_LastHand = -1;
static int s_SameHandCount = 0;
static int s_ActivePatternType = 0;
static int s_PatternSubIndex = 0;

static float s_LastNoteTimeByLane[4] = { -10.0f, -10.0f, -10.0f, -10.0f };

// ============================================================================
// 수동 레코딩용 잔잔한 4~6노트 단위 패턴 풀 (45가지 기본 패턴)
// 조합에 따라 수백~수만 가지의 자연스러운 플레이 흐름을 만듭니다.
// ============================================================================
static const std::vector<std::vector<int>> EASY_PATTERN_POOL = {
    // [1. 계단 & 파도형]
    {0, 1, 2, 3}, {3, 2, 1, 0}, {0, 1, 2, 1}, {3, 2, 1, 2},
    {1, 2, 3, 2}, {2, 1, 0, 1}, {0, 1, 3, 2}, {3, 2, 0, 1},
    {1, 0, 2, 3}, {2, 3, 1, 0}, {0, 2, 1, 2}, {3, 1, 2, 1},

    // [2. 양손 번갈아 치기 (Trill/Alternating)]
    {0, 2, 1, 3}, {1, 3, 0, 2}, {0, 3, 1, 2}, {1, 2, 0, 3},
    {2, 0, 3, 1}, {3, 1, 2, 0}, {2, 1, 3, 0}, {3, 0, 2, 1},
    {0, 2, 1, 2}, {3, 1, 2, 0}, {1, 3, 2, 0}, {2, 0, 1, 3},

    // [3. 피벗 & 잔잔한 반복]
    {0, 2, 0, 3}, {3, 1, 3, 0}, {1, 3, 1, 0}, {2, 0, 2, 3},
    {0, 1, 0, 2}, {3, 2, 3, 1}, {1, 0, 1, 3}, {2, 3, 2, 0},
    {0, 3, 0, 1}, {3, 0, 3, 2}, {1, 2, 1, 0}, {2, 1, 2, 3},

    // [4. 지그재그 & 부드러운 도약]
    {0, 2, 3, 1}, {3, 1, 0, 2}, {1, 0, 3, 2}, {2, 3, 0, 1},
    {0, 1, 3, 1}, {3, 2, 0, 2}, {1, 2, 0, 2}, {2, 1, 3, 1},

    // [5. 6노트 긴 잔잔한 흐름]
    {0, 1, 2, 3, 2, 1}, {3, 2, 1, 0, 1, 2},
    {0, 2, 1, 3, 2, 0}, {3, 1, 2, 0, 1, 3},
    {1, 0, 2, 1, 3, 2}, {2, 3, 1, 2, 0, 1},
    {0, 1, 0, 2, 1, 3}, {3, 2, 3, 1, 2, 0}
};

static int s_ManualPatternIndex = -1;
static int s_ManualPatternStep = 0;

static float ApplyQuantizeFilter(float rawTimeSec, float beatIntervalSec, float& outSnapErrorMs) {
    float interval16 = beatIntervalSec / 4.0f;
    float interval12 = beatIntervalSec / 3.0f;

    float relTime = rawTimeSec - s_SongOffsetSec;
    if (relTime < 0.0f) relTime = 0.0f;

    float index16 = roundf(relTime / interval16);
    float snapTime16 = s_SongOffsetSec + (index16 * interval16);
    float error16Ms = fabsf(relTime - snapTime16) * 1000.0f;

    float index12 = roundf(relTime / interval12);
    float snapTime12 = s_SongOffsetSec + (index12 * interval12);
    float error12Ms = fabsf(relTime - snapTime12) * 1000.0f;

    bool is16BitGrid = (error16Ms <= error12Ms);
    float nearestBeatTime = is16BitGrid ? snapTime16 : snapTime12;
    float bestSnapErrorMs = is16BitGrid ? error16Ms : error12Ms;

    outSnapErrorMs = bestSnapErrorMs;

    if (bestSnapErrorMs <= s_QuantizeThresholdMs) {
        return nearestBeatTime;
    }
    return rawTimeSec;
}

static void LoadSuitFontWithKorean() {
    if (s_IsFontLoaded) return;
    
    std::string fontPath = "fonts/SUIT-Heavy.ttf";
    if (std::filesystem::exists(fontPath)) {
        std::vector<int> codepoints;
        for (int i = 32; i <= 126; ++i) codepoints.push_back(i);
        for (int i = 0xAC00; i <= 0xD7A3; ++i) codepoints.push_back(i);
        for (int i = 0x3131; i <= 0x318E; ++i) codepoints.push_back(i);

        s_SuitFont = LoadFontEx(fontPath.c_str(), 32, codepoints.data(), (int)codepoints.size());
        s_IsFontLoaded = true;
    } else {
        s_SuitFont = GetFontDefault();
        s_IsFontLoaded = true;
    }
}

static void SetupFmodFft(MusicExecute::MusicPlayer1* musicPlayer, AudioManager* audioManager) {
    if (!musicPlayer) return;

    FMOD_CHANNEL* rawChannel = musicPlayer->GetChannelRaw();
    if (rawChannel) {
        FMOD_SYSTEM* fmodSys = nullptr;
        FMOD_Channel_GetSystemObject(rawChannel, &fmodSys);

        if (fmodSys && !s_FftDsp) {
            FMOD_System_CreateDSPByType(fmodSys, FMOD_DSP_TYPE_FFT, &s_FftDsp);
            if (s_FftDsp) {
                FMOD_DSP_SetParameterInt(s_FftDsp, FMOD_DSP_FFT_WINDOWSIZE, 512);
                FMOD_DSP_SetParameterInt(s_FftDsp, FMOD_DSP_FFT_WINDOW, FMOD_DSP_FFT_WINDOW_BLACKMAN);
            }
        }

        if (s_FftDsp && !s_IsFftInitialized) {
            FMOD_Channel_AddDSP(rawChannel, 0, s_FftDsp);
            s_IsFftInitialized = true;
        }
    }
}

ChartEditor::ChartEditor() 
    : scrollOffset(0.0f) {
    m_MusicPlayer = new MusicExecute::MusicPlayer1();
    m_AudioManager = new AudioManager();
}

ChartEditor::~ChartEditor() {
    Release();
}

void ChartEditor::Init() {
    scrollOffset = 0.0f;
    s_IsRecording = false;
    s_IsAutoRecording = false;
    s_IsTestPlaying = false;
    s_CurrentPitch = 1.0f;
    s_ScrollSpeed = 200.0f;
    s_IsSpeedInputActive = false;
    s_SpeedInputString.clear();
    s_IsMusicSelectOpen = false;
    s_CurrentMusicPath = "";
    s_IsFftInitialized = false;
    s_PrevEnergy = 0.0f;
    s_LastAutoNoteTime = -1.0f;
    s_PatternStep = 0;
    s_LastAssignedLane = -1;
    s_PatternDirection = 1;
    s_EnergyHistoryIndex = 0;
    s_EnergyHistoryCount = 0;
    s_IsAudioStarted = false;
    s_SameLaneCount = 0;
    s_LastHand = -1;
    s_SameHandCount = 0;
    s_ActivePatternType = 0;
    s_PatternSubIndex = 0;
    s_ManualPatternIndex = -1;
    s_ManualPatternStep = 0;
    for (int i = 0; i < 4; ++i) s_LastNoteTimeByLane[i] = -10.0f;
    std::fill(std::begin(s_EnergyHistory), std::end(s_EnergyHistory), 0.0f);
    s_EditorNotes.clear();
    notes.clear();

    LoadSuitFontWithKorean();

    if (m_AudioManager->Init()) {
        m_AudioManager->Update();
    }
}

void ChartEditor::HandleInput() {
    m_AudioManager->Update();

    if (m_MusicPlayer) {
        m_MusicPlayer->Update(GetFrameTime());
    }

    for (int i = 0; i < 6; ++i) {
        if (s_LanePulseTimers[i] > 0.0f) {
            s_LanePulseTimers[i] -= GetFrameTime();
            if (s_LanePulseTimers[i] < 0.0f) s_LanePulseTimers[i] = 0.0f;
        }
    }

    if (s_IsRecording) {
        if (!m_MusicPlayer) return;

        if (IsKeyPressed(KEY_U)) {
            s_IsRecording = false;
            s_IsAutoRecording = false;
            m_MusicPlayer->Stop();
            return;
        }

        float currentSec = (float)m_MusicPlayer->GetCurrentPositionMs() / 1000.0f;
        if (currentSec < 0.0f) currentSec = 0.0f;
        scrollOffset = currentSec * s_ScrollSpeed;

        if (s_IsAutoRecording && s_FftDsp) {
            void* fftData = nullptr;
            unsigned int dataLen = 0;
            if (FMOD_DSP_GetParameterData(s_FftDsp, FMOD_DSP_FFT_SPECTRUMDATA, &fftData, &dataLen, nullptr, 0) == FMOD_OK && fftData != nullptr) {
                FMOD_DSP_PARAMETER_FFT* fft = (FMOD_DSP_PARAMETER_FFT*)fftData;
                if (fft->numchannels > 0) {
                    float lowEnergy = 0.0f;
                    float midEnergy = 0.0f;
                    float highEnergy = 0.0f;

                    for (int i = 0; i < 4; ++i) lowEnergy += fft->spectrum[0][i];
                    for (int i = 4; i < 12; ++i) midEnergy += fft->spectrum[0][i];
                    for (int i = 12; i < 32; ++i) highEnergy += fft->spectrum[0][i];

                    float totalEnergy = lowEnergy + midEnergy + highEnergy;
                    float energyDelta = totalEnergy - s_PrevEnergy;

                    if (!s_IsAudioStarted) {
                        if (energyDelta > 0.15f || (totalEnergy >= 0.35f && energyDelta > 0.10f)) {
                            s_IsAudioStarted = true;
                        }
                    }

                    if (s_IsAudioStarted) {
                        s_EnergyHistory[s_EnergyHistoryIndex] = totalEnergy;
                        s_EnergyHistoryIndex = (s_EnergyHistoryIndex + 1) % ENERGY_HISTORY_SIZE;
                        if (s_EnergyHistoryCount < ENERGY_HISTORY_SIZE) s_EnergyHistoryCount++;

                        float avgEnergy = 0.0f;
                        for (int i = 0; i < s_EnergyHistoryCount; ++i) {
                            avgEnergy += s_EnergyHistory[i];
                        }
                        avgEnergy /= (float)s_EnergyHistoryCount;

                        bool isBurstMode = (s_EnergyHistoryCount >= 5) && (totalEnergy > avgEnergy * 2.0f) && (avgEnergy > 0.003f);
                        float dynamicThreshold = isBurstMode ? (avgEnergy * 1.05f + 0.005f) : (avgEnergy * 1.25f + 0.010f);

                        float beatIntervalSec = 60.0f / s_EditorBPM;
                        float bestSnapErrorMs = 0.0f;
                        float nearestBeatTime = ApplyQuantizeFilter(currentSec, beatIntervalSec, bestSnapErrorMs);
                        float snappedWorldY = nearestBeatTime * s_ScrollSpeed;

                        float interval16 = beatIntervalSec / 4.0f;
                        float minNoteIntervalSec = isBurstMode ? (interval16 * 0.40f) : (interval16 * 0.60f);

                        bool isOnset = (totalEnergy > dynamicThreshold && energyDelta > 0.02f) || (isBurstMode && energyDelta > 0.010f);

                        if (isOnset && (s_LastAutoNoteTime < 0.0f || (nearestBeatTime - s_LastAutoNoteTime >= minNoteIntervalSec))) {
                            bool duplicate = false;
                            for (const auto& note : s_EditorNotes) {
                                if (fabsf(note.posX - snappedWorldY) < 1.0f) {
                                    duplicate = true;
                                    break;
                                }
                            }

                            if (!duplicate) {
                                bool isLowBpmMode = (s_EditorBPM < 150.0f);
                                bool isLaurHardcoreKick = (lowEnergy > midEnergy * 1.30f) && (lowEnergy > highEnergy * 1.30f) && (totalEnergy > avgEnergy * 1.30f);
                                bool isHighMelody = (highEnergy > midEnergy * 1.15f) && (highEnergy > lowEnergy * 1.15f);

                                if (!isLowBpmMode && (isLaurHardcoreKick || (isBurstMode && lowEnergy > avgEnergy * 1.70f))) {
                                    static bool s_AlternateChord = false;
                                    int laneA = s_AlternateChord ? 1 : 0;
                                    int laneB = s_AlternateChord ? 2 : 3;
                                    s_AlternateChord = !s_AlternateChord;

                                    Color kickColor = isBurstMode ? RED : PURPLE;
                                    s_EditorNotes.push_back({ laneA, snappedWorldY, bestSnapErrorMs, kickColor });
                                    s_EditorNotes.push_back({ laneB, snappedWorldY, bestSnapErrorMs, kickColor });
                                    s_LanePulseTimers[laneA] = 0.15f;
                                    s_LanePulseTimers[laneB] = 0.15f;

                                    s_LastAssignedLane = -1;
                                    s_SameLaneCount = 0;
                                    s_SameHandCount = 0;
                                    s_LastHand = -1;
                                } else {
                                    bool isFastStream = !isLowBpmMode && (isBurstMode || (s_LastAutoNoteTime > 0.0f && (nearestBeatTime - s_LastAutoNoteTime <= interval16 * 1.25f)));
                                    int candidateLane = 0;

                                    if (isLowBpmMode) {
                                        s_PatternSubIndex = 0;
                                        int targetHand = (s_LastHand == 0) ? 1 : 0;
                                        if (targetHand == 0) {
                                            candidateLane = (s_LastAssignedLane == 0) ? 1 : 0;
                                        } else {
                                            candidateLane = (s_LastAssignedLane == 2) ? 3 : 2;
                                        }
                                    } else if (isFastStream) {
                                        static const int PATTERNS[3][4] = {
                                            { 0, 1, 2, 3 },
                                            { 3, 2, 1, 0 },
                                            { 1, 2, 1, 2 }
                                        };
                                        candidateLane = PATTERNS[s_ActivePatternType][s_PatternSubIndex];
                                        s_PatternSubIndex = (s_PatternSubIndex + 1) % 4;
                                        if (s_PatternSubIndex == 0) {
                                            s_ActivePatternType = (s_ActivePatternType + 1) % 3;
                                        }
                                    } else {
                                        s_PatternSubIndex = 0;
                                        if (isHighMelody) {
                                            candidateLane = (s_LastAssignedLane == 1) ? 2 : 1;
                                        } else {
                                            candidateLane = (s_LastAssignedLane + s_PatternDirection + LANE_COUNT) % LANE_COUNT;
                                        }
                                    }

                                    int candidateHand = (candidateLane < 2) ? 0 : 1;
                                    if (s_LastHand != -1 && s_SameHandCount >= 3 && candidateHand == s_LastHand) {
                                        int forcedHand = 1 - s_LastHand;
                                        if (forcedHand == 0) {
                                            candidateLane = (candidateLane == 2) ? 1 : 0;
                                        } else {
                                            candidateLane = (candidateLane == 0) ? 3 : 2;
                                        }
                                        candidateHand = forcedHand;
                                    }

                                    if (candidateLane == s_LastAssignedLane && s_SameLaneCount >= 2) {
                                        for (int offset = 1; offset < LANE_COUNT; ++offset) {
                                            int testLane = (candidateLane + offset) % LANE_COUNT;
                                            int testHand = (testLane < 2) ? 0 : 1;
                                            if (s_LastHand != -1 && s_SameHandCount >= 3 && testHand == s_LastHand) {
                                                continue;
                                            }
                                            if (testLane != s_LastAssignedLane) {
                                                candidateLane = testLane;
                                                candidateHand = testHand;
                                                break;
                                            }
                                        }
                                    }

                                    if (candidateLane == s_LastAssignedLane) {
                                        s_SameLaneCount++;
                                    } else {
                                        s_SameLaneCount = 1;
                                    }

                                    if (candidateHand == s_LastHand) {
                                        s_SameHandCount++;
                                    } else {
                                        s_SameHandCount = 1;
                                        s_LastHand = candidateHand;
                                    }

                                    s_LastAssignedLane = candidateLane;

                                    Color noteColor = (bestSnapErrorMs <= s_QuantizeThresholdMs) ? SKYBLUE : ORANGE;
                                    if (isHighMelody) noteColor = (bestSnapErrorMs <= s_QuantizeThresholdMs) ? GOLD : VIOLET;
                                    if (isBurstMode) noteColor = YELLOW;

                                    s_EditorNotes.push_back({ candidateLane, snappedWorldY, bestSnapErrorMs, noteColor });
                                    s_LanePulseTimers[candidateLane] = 0.15f;
                                }

                                s_PatternStep++;
                                notes.clear();
                                for (const auto& n : s_EditorNotes) {
                                    notes.push_back({ n.lane, n.posX });
                                }
                                s_LastAutoNoteTime = nearestBeatTime;
                            }
                        }
                    }
                    s_PrevEnergy = totalEnergy;
                }
            }
        } else if (!s_IsAutoRecording) {
            // 요청 키: 1, 2, E, \, INS, HOME
            int recordKeys[] = { 
                KEY_ONE, KEY_TWO, KEY_E, KEY_BACKSLASH, 
                KEY_INSERT, KEY_HOME
            };

            bool anyKeyPressed = false;
            for (int idx = 0; idx < 6; ++idx) {
                if (IsKeyPressed(recordKeys[idx])) {
                    anyKeyPressed = true;
                    break;
                }
            }

            if (anyKeyPressed) {
                if (!s_IsAudioStarted) {
                    s_IsAudioStarted = true;
                }

                float exactInputTime = currentSec - s_InputLatency;
                if (exactInputTime < 0.0f) exactInputTime = 0.0f;

                float beatIntervalSec = 60.0f / s_EditorBPM;
                float bestSnapErrorMs = 0.0f;
                float nearestBeatTime = ApplyQuantizeFilter(exactInputTime, beatIntervalSec, bestSnapErrorMs);
                float snappedWorldY = nearestBeatTime * s_ScrollSpeed;

                // [동타 및 중복 방지]: 해당 비트 위치에 이미 노트가 있으면 무시
                bool beatAlreadyHasNote = false;
                for (const auto& note : s_EditorNotes) {
                    if (fabsf(note.posX - snappedWorldY) < 1.0f) {
                        beatAlreadyHasNote = true;
                        break;
                    }
                }

                if (!beatAlreadyHasNote) {
                    // 패턴 체이닝 엔진: 새로운 잔잔한 패턴 선택 및 경계 연타 방지
                    if (s_ManualPatternIndex < 0 || s_ManualPatternStep >= (int)EASY_PATTERN_POOL[s_ManualPatternIndex].size()) {
                        std::vector<int> validPatternIndices;
                        
                        // 이전 패턴과 이어질 때 연타(폭타)가 발생하지 않는 패턴만 무작위 선택
                        for (size_t p = 0; p < EASY_PATTERN_POOL.size(); ++p) {
                            if (s_LastAssignedLane == -1 || EASY_PATTERN_POOL[p][0] != s_LastAssignedLane) {
                                validPatternIndices.push_back((int)p);
                            }
                        }

                        if (!validPatternIndices.empty()) {
                            s_ManualPatternIndex = validPatternIndices[rand() % validPatternIndices.size()];
                        } else {
                            s_ManualPatternIndex = rand() % EASY_PATTERN_POOL.size();
                        }
                        s_ManualPatternStep = 0;
                    }

                    int targetLane = EASY_PATTERN_POOL[s_ManualPatternIndex][s_ManualPatternStep++];
                    s_LastAssignedLane = targetLane;

                    Color noteColor = (targetLane % 2 == 0) ? SKYBLUE : ORANGE;
                    if (bestSnapErrorMs <= s_QuantizeThresholdMs) noteColor = GOLD;

                    s_EditorNotes.push_back({ targetLane, snappedWorldY, bestSnapErrorMs, noteColor });
                    s_LanePulseTimers[targetLane] = 0.15f;

                    notes.clear();
                    for (const auto& n : s_EditorNotes) {
                        notes.push_back({ n.lane, n.posX });
                    }
                }
            }
        }

        return;
    }

    if (s_IsTestPlaying) {
        if (IsKeyPressed(KEY_U)) {
            s_IsTestPlaying = false;
            m_MusicPlayer->Stop(); 
            return;
        }
        if (IsKeyPressed(KEY_ENTER)) {
            s_IsTestPlaying = false;
            if (m_MusicPlayer) {
                m_MusicPlayer->Stop();
            }
            return;
        }
        s_EditorPlay.Update(s_IsTestPlaying);
        return;
    }

    if (IsKeyPressed(KEY_Q)) {
        s_MusicFileList.clear();
        if (std::filesystem::exists("music")) {
            for (const auto& entry : std::filesystem::directory_iterator("music")) {
                if (entry.is_regular_file()) {
                    s_MusicFileList.push_back(entry.path().string());
                }
            }
        }
        if (s_MusicFileList.empty()) {
            s_MusicFileList.push_back("music");
        }
        s_SelectedMusicIndex = 0;
        s_IsMusicSelectOpen = true;
    }

    if (IsKeyPressed(KEY_T)) {
        s_IsSpeedInputActive = !s_IsSpeedInputActive;
        if (s_IsSpeedInputActive) {
            s_SpeedInputString = std::to_string((int)s_ScrollSpeed);
        }
    }

    if (s_IsSpeedInputActive) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= '0' && key <= '9') {
                s_SpeedInputString.push_back((char)key);
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (!s_SpeedInputString.empty()) {
                s_SpeedInputString.pop_back();
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (!s_SpeedInputString.empty()) {
                s_ScrollSpeed = (float)std::stoi(s_SpeedInputString);
            }
            s_IsSpeedInputActive = false;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            s_IsSpeedInputActive = false;
        }
        return;
    }

    if (s_IsMusicSelectOpen) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        Vector2 mousePos = GetMousePosition();

        float boxWidth = 450.f;
        float boxHeight = 60.f + s_MusicFileList.size() * 45.f;
        if (boxHeight < 100.f) boxHeight = 100.f;
        if (boxHeight > 500.f) boxHeight = 500.f;

        Rectangle box = { (float)screenWidth / 2.f - boxWidth / 2.f, (float)screenHeight / 2.f - boxHeight / 2.f, boxWidth, boxHeight };

        float startY = box.y + 30.f;
        for (size_t i = 0; i < s_MusicFileList.size(); ++i) { // size_t로 올바르게 사용
            Rectangle itemBox = { box.x + 20.f, startY + (float)(i * 45.f), boxWidth - 40.f, 35.f };
            if (CheckCollisionPointRec(mousePos, itemBox)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    s_SelectedMusicIndex = (int)i;
                }
            }
        }

        if (IsKeyPressed(KEY_DOWN)) {
            s_SelectedMusicIndex = (s_SelectedMusicIndex + 1) % (int)s_MusicFileList.size();
        }
        if (IsKeyPressed(KEY_UP)) {
            s_SelectedMusicIndex = (s_SelectedMusicIndex - 1 + s_MusicFileList.size()) % (int)s_MusicFileList.size();
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (!s_MusicFileList.empty() && s_SelectedMusicIndex >= 0 && s_SelectedMusicIndex < (int)s_MusicFileList.size()) {
                s_CurrentMusicPath = s_MusicFileList[s_SelectedMusicIndex];
                
                if (m_MusicPlayer) {
                    m_MusicPlayer->Stop();
                    delete m_MusicPlayer;
                    m_MusicPlayer = new MusicExecute::MusicPlayer1();
                    m_MusicPlayer->InitializeWithCustomPath(*m_AudioManager, "Music1", s_CurrentMusicPath);
                    s_IsFftInitialized = false;
                }
            }
            s_IsMusicSelectOpen = false;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            s_IsMusicSelectOpen = false;
        }
        return;
    }

    static std::string loadedMusicPath = s_CurrentMusicPath;
    static std::vector<SaveNoteData> loadedNotes;
    static bool fileLoaded = false;

    if (fileLoaded) {
        s_EditorNotes.clear();
        notes.clear();
        for (const auto& n : loadedNotes) {
            s_EditorNotes.push_back({ n.lane, n.posX, 0.0f, WHITE });
            notes.push_back({ n.lane, n.posX });
        }
        fileLoaded = false;
    }

    std::vector<SaveNoteData> currentSaveData;
    for (size_t i = 0; i < s_EditorNotes.size(); ++i) {
        currentSaveData.push_back({ s_EditorNotes[i].lane, s_EditorNotes[i].posX });
    }

    ChartSave::HandleChartInput(
        s_CurrentMusicPath, 
        currentSaveData, 
        loadedMusicPath, 
        loadedNotes, 
        fileLoaded
    );

    if (ChartSave::IsPopupOpen()) {
        return;
    }

    if (IsKeyPressed(KEY_THREE)) {
        s_CurrentPitch -= 0.05f;
        if (s_CurrentPitch < 0.25f) s_CurrentPitch = 0.25f;
        if (m_MusicPlayer) m_MusicPlayer->SetPitch(s_CurrentPitch);
    }

    if (IsKeyPressed(KEY_FOUR)) {
        s_CurrentPitch += 0.05f;
        if (s_CurrentPitch > 1.0f) s_CurrentPitch = 1.0f;
        if (m_MusicPlayer) m_MusicPlayer->SetPitch(s_CurrentPitch);
    }

    if (IsKeyPressed(KEY_A)) {
        if (s_CurrentMusicPath.empty()) return;
        s_EditorNotes.clear();
        notes.clear();
        scrollOffset = 0.0f;
        s_PrevEnergy = 0.0f;
        s_LastAutoNoteTime = -1.0f;
        s_PatternStep = 0;
        s_LastAssignedLane = -1;
        s_PatternDirection = 1;
        s_EnergyHistoryIndex = 0;
        s_EnergyHistoryCount = 0;
        s_IsAudioStarted = false;
        s_SameLaneCount = 0;
        s_LastHand = -1;
        s_SameHandCount = 0;
        s_ActivePatternType = 0;
        s_PatternSubIndex = 0;
        s_ManualPatternIndex = -1;
        s_ManualPatternStep = 0;
        for (int i = 0; i < 4; ++i) s_LastNoteTimeByLane[i] = -10.0f;
        std::fill(std::begin(s_EnergyHistory), std::end(s_EnergyHistory), 0.0f);
        m_MusicPlayer->Stop();
        m_MusicPlayer->Play(*m_AudioManager, 0);
        m_MusicPlayer->PlayImmediate();
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
        s_IsRecording = true;
        s_IsAutoRecording = true;
        SetupFmodFft(m_MusicPlayer, m_AudioManager);
        return;
    }

    if (IsKeyPressed(KEY_SEVEN)) {
        if (s_CurrentMusicPath.empty()) return;
        s_EditorNotes.clear();
        notes.clear();
        scrollOffset = 0.0f;
        s_PrevEnergy = 0.0f;
        s_LastAutoNoteTime = -1.0f;
        s_PatternStep = 0;
        s_LastAssignedLane = -1;
        s_PatternDirection = 1;
        s_EnergyHistoryIndex = 0;
        s_EnergyHistoryCount = 0;
        s_IsAudioStarted = false;
        s_SameLaneCount = 0;
        s_LastHand = -1;
        s_SameHandCount = 0;
        s_ActivePatternType = 0;
        s_PatternSubIndex = 0;
        s_ManualPatternIndex = -1;
        s_ManualPatternStep = 0;
        for (int i = 0; i < 4; ++i) s_LastNoteTimeByLane[i] = -10.0f;
        std::fill(std::begin(s_EnergyHistory), std::end(s_EnergyHistory), 0.0f);
        m_MusicPlayer->Stop();
        m_MusicPlayer->Play(*m_AudioManager, 0);
        m_MusicPlayer->PlayImmediate();
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
        s_IsRecording = true;
        s_IsAutoRecording = false;
        SetupFmodFft(m_MusicPlayer, m_AudioManager);
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        if (s_CurrentMusicPath.empty()) return;
        Texture2D dummyTex = { 0 };
        s_EditorPlay.Init(currentSaveData, dummyTex, m_MusicPlayer);
        
        m_MusicPlayer->Stop();
        m_MusicPlayer->Play(*m_AudioManager, 0);
        m_MusicPlayer->PlayImmediate();
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
        
        s_IsTestPlaying = true;
        SetupFmodFft(m_MusicPlayer, m_AudioManager);
        return;
    }

    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f) {
        scrollOffset += wheelMove * 40.0f;
    }

    if (scrollOffset < 0.0f) {
        scrollOffset = 0.0f;
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (!s_EditorNotes.empty()) {
            s_EditorNotes.pop_back();
            notes.clear();
            for (const auto& n : s_EditorNotes) {
                notes.push_back({ n.lane, n.posX });
            }
        }
    }
}

void ChartEditor::Render() {
    if (s_IsTestPlaying) {
        s_EditorPlay.Draw();
        return;
    }

    if (s_IsRecording && m_MusicPlayer) {
        float currentSec = (float)m_MusicPlayer->GetCurrentPositionMs() / 1000.0f;
        scrollOffset = currentSec * s_ScrollSpeed;

        float beatInterval = 60.0f / s_EditorBPM;
        float beatPhase = fmodf(currentSec, beatInterval) / beatInterval;
        if (beatPhase < 0.2f) {
            s_MetronomeFlashAlpha = (1.0f - (beatPhase / 0.2f)) * 0.15f;
        } else {
            s_MetronomeFlashAlpha = 0.0f;
        }
    } else {
        s_MetronomeFlashAlpha = 0.0f;
    }

    DrawRectangle(0, 0, 1280, 720, Color{ 8, 10, 15, 255 });
    DrawRectangleGradientV(0, 0, 1280, 720, Color{ 25, 18, 45, 120 }, Color{ 5, 10, 20, 220 });

    if (s_MetronomeFlashAlpha > 0.0f) {
        DrawRectangle((int)PLAYFIELD_X, (int)PLAYFIELD_Y, (int)PLAYFIELD_WIDTH, (int)PLAYFIELD_HEIGHT, Fade(SKYBLUE, s_MetronomeFlashAlpha * 0.8f));
    }

    float leftVisualizerX = 40.0f;
    float leftVisualizerY = 20.0f;
    float leftVisualizerW = 600.0f;
    float leftVisualizerH = 680.0f;

    DrawRectangleRounded(Rectangle{ leftVisualizerX, leftVisualizerY, leftVisualizerW, leftVisualizerH }, 0.025f, 4, Color{ 15, 18, 28, 240 });
    DrawRectangleRoundedLines(Rectangle{ leftVisualizerX, leftVisualizerY, leftVisualizerW, leftVisualizerH }, 0.025f, 4, Fade(SKYBLUE, 0.45f));
    
    DrawLineEx({ leftVisualizerX, leftVisualizerY + 15.0f }, { leftVisualizerX + 15.0f, leftVisualizerY }, 2.0f, SKYBLUE);
    DrawLineEx({ leftVisualizerX + leftVisualizerW - 15.0f, leftVisualizerY }, { leftVisualizerX + leftVisualizerW, leftVisualizerY + 15.0f }, 2.0f, SKYBLUE);

    DrawLineEx({ leftVisualizerX + 20.0f, leftVisualizerY + 75.0f }, { leftVisualizerX + leftVisualizerW - 20.0f, leftVisualizerY + 75.0f }, 1.5f, Fade(SKYBLUE, 0.4f));
    DrawCircle((int)(leftVisualizerX + leftVisualizerW - 35.0f), (int)(leftVisualizerY + 45.0f), 4.0f, Fade(GREEN, 0.8f));

    DrawTextEx(s_SuitFont, editorTexts[0].c_str(), Vector2{ leftVisualizerX + 25.0f, leftVisualizerY + 25.0f }, 18.0f, 1.0f, RAYWHITE);
    DrawTextEx(s_SuitFont, editorTexts[1].c_str(), Vector2{ leftVisualizerX + 25.0f, leftVisualizerY + 50.0f }, 12.0f, 1.0f, SKYBLUE);

    if (s_FftDsp && m_MusicPlayer) {
        if (!s_IsFftInitialized) {
            SetupFmodFft(m_MusicPlayer, m_AudioManager);
        }

        void* fftData = nullptr;
        unsigned int dataLen = 0;
        if (FMOD_DSP_GetParameterData(s_FftDsp, FMOD_DSP_FFT_SPECTRUMDATA, &fftData, &dataLen, nullptr, 0) == FMOD_OK && fftData != nullptr) {
            FMOD_DSP_PARAMETER_FFT* fft = (FMOD_DSP_PARAMETER_FFT*)fftData;
            if (fft->numchannels > 0) {
                int barCount = 32;
                float startX = leftVisualizerX + 35.0f;
                float baseY = leftVisualizerY + 520.0f;
                float totalWidth = leftVisualizerW - 70.0f;
                float barWidth = (totalWidth / (float)barCount) - 4.0f;

                DrawLineEx({ startX - 10.0f, baseY }, { startX + totalWidth + 10.0f, baseY }, 1.0f, Fade(SKYBLUE, 0.3f));

                for (int i = 0; i < barCount; ++i) {
                    float amplitude = fft->spectrum[0][i];
                    float barHeight = std::clamp(amplitude * 450.0f, 2.0f, 380.0f);

                    Color barColor = ColorFromHSV((float)i * 10.0f + 180.0f, 0.85f, 0.95f);
                    
                    DrawRectangleRounded(
                        Rectangle{ startX + (float)i * (barWidth + 4.0f), baseY - barHeight, barWidth, barHeight }, 
                        0.4f, 2, barColor
                    );
                    DrawRectangleRounded(
                        Rectangle{ startX + (float)i * (barWidth + 4.0f), baseY - barHeight, barWidth, 3.0f }, 
                        0.4f, 2, RAYWHITE
                    );
                }
            }
        }
    } else {
        DrawRectangleRounded(Rectangle{ leftVisualizerX + 50.0f, leftVisualizerY + 180.0f, leftVisualizerW - 100.0f, 260.0f }, 0.05f, 4, Color{ 20, 24, 38, 150 });
        DrawRectangleRoundedLines(Rectangle{ leftVisualizerX + 50.0f, leftVisualizerY + 180.0f, leftVisualizerW - 100.0f, 260.0f }, 0.05f, 4, Fade(SKYBLUE, 0.2f));
        
        for (int k = 0; k < 5; ++k) {
            float lineY = leftVisualizerY + 220.0f + (k * 40.0f);
            DrawLineEx({ leftVisualizerX + 80.0f, lineY }, { leftVisualizerX + leftVisualizerW - 80.0f, lineY }, 1.0f, Fade(SKYBLUE, 0.08f));
        }

        DrawTextEx(s_SuitFont, editorTexts[2].c_str(), Vector2{ leftVisualizerX + 85.0f, leftVisualizerY + 295.0f }, 13.0f, 1.0f, Fade(SKYBLUE, 0.8f));
    }

    std::string pitchStr = editorTexts[3] + std::to_string((int)(s_CurrentPitch * 100.0f)) + "% (3/4 키 사용)";
    DrawTextEx(s_SuitFont, pitchStr.c_str(), Vector2{ leftVisualizerX + 25.0f, leftVisualizerY + 625.0f }, 14.0f, 1.0f, LIGHTGRAY);

    DrawRectangleRounded(Rectangle{ PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT }, 0.025f, 4, Color{ 12, 15, 24, 245 });
    DrawRectangleRoundedLines(Rectangle{ PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT }, 0.025f, 4, Fade(PURPLE, 0.6f));

    for (int i = 0; i < LANE_COUNT; ++i) {
        float laneX = LANE_START_X + (LANE_WIDTH * i);
        DrawRectangle((int)laneX, 0, (int)LANE_WIDTH, (int)JUDGMENT_LINE_Y, Fade(SKYBLUE, i % 2 == 0 ? 0.035f : 0.012f));
        if (i > 0) {
            DrawLine((int)laneX, 0, (int)laneX, (int)JUDGMENT_LINE_Y, Fade(PURPLE, 0.25f));
        }
    }

    float gridSize = 50.0f;
    float startWorldY = scrollOffset - JUDGMENT_LINE_Y;
    if (startWorldY < 0.0f) startWorldY = 0.0f;
    float endWorldY = scrollOffset + JUDGMENT_LINE_Y;

    float subGridSize = gridSize / 4.0f;
    float firstGridY = ceilf(startWorldY / subGridSize) * subGridSize;
    
    for (float worldY = firstGridY; worldY <= endWorldY; worldY += subGridSize) {
        float screenY = JUDGMENT_LINE_Y - (worldY - scrollOffset);
        if (screenY >= 0.0f && screenY <= JUDGMENT_LINE_Y) {
            bool isBarLine = (int(roundf(worldY / subGridSize)) % 16) == 0;
            bool isBeatLine = (int(roundf(worldY / subGridSize)) % 4) == 0;

            float alpha = isBarLine ? 0.35f : (isBeatLine ? 0.18f : 0.06f);
            float thickness = isBarLine ? 1.5f : 1.0f;
            Color gridColor = isBarLine ? SKYBLUE : (isBeatLine ? WHITE : Fade(WHITE, 0.15f));

            DrawLineEx(
                { LANE_START_X, screenY }, 
                { LANE_START_X + LANE_AREA_WIDTH, screenY }, 
                thickness, Fade(gridColor, alpha)
            );
        }
    }

    if (m_MusicPlayer) {
        float currentSec = (float)m_MusicPlayer->GetCurrentPositionMs() / 1000.0f;
        float musicWorldY = currentSec * s_ScrollSpeed;
        float syncLineScreenY = JUDGMENT_LINE_Y - (musicWorldY - scrollOffset);

        if (syncLineScreenY >= 0.0f && syncLineScreenY <= JUDGMENT_LINE_Y) {
            DrawLineEx(
                { LANE_START_X, syncLineScreenY - 1.0f }, 
                { LANE_START_X + LANE_AREA_WIDTH, syncLineScreenY - 1.0f }, 
                3.0f, Fade(YELLOW, 0.4f)
            );
            DrawLineEx(
                { LANE_START_X, syncLineScreenY }, 
                { LANE_START_X + LANE_AREA_WIDTH, syncLineScreenY }, 
                1.5f, Fade(YELLOW, 0.8f)
            );
        }
    }

    DrawLineEx({ LANE_START_X - 6.0f, JUDGMENT_LINE_Y }, { LANE_START_X + LANE_AREA_WIDTH + 6.0f, JUDGMENT_LINE_Y }, 5.0f, Fade(PURPLE, 0.7f));
    DrawLineEx({ LANE_START_X, JUDGMENT_LINE_Y }, { LANE_START_X + LANE_AREA_WIDTH, JUDGMENT_LINE_Y }, 2.0f, RAYWHITE);

    for (const auto& note : s_EditorNotes) {
        float screenNoteY = JUDGMENT_LINE_Y - (note.posX - scrollOffset);
        if (screenNoteY >= -30.0f && screenNoteY <= JUDGMENT_LINE_Y + 30.0f) {
            float noteX = LANE_X_COORDS[note.lane];
            DrawRectangleRounded(Rectangle{ noteX - 28.0f, screenNoteY - 6.0f, 56.0f, 12.0f }, 0.4f, 4, Fade(note.feedbackColor, 0.4f));
            DrawRectangleRounded(Rectangle{ noteX - 26.0f, screenNoteY - 5.0f, 52.0f, 10.0f }, 0.3f, 4, note.feedbackColor);
            DrawRectangleRounded(Rectangle{ noteX - 22.0f, screenNoteY - 2.0f, 44.0f, 4.0f }, 0.3f, 4, Color{ 20, 20, 20, 255 });
        }
    }

    float panelY = 636.0f;
    float panelH = 72.0f;
    DrawRectangleRounded(Rectangle{ PLAYFIELD_X + 6.0f, panelY, PLAYFIELD_WIDTH - 12.0f, panelH }, 0.15f, 4, Color{ 18, 22, 35, 250 });
    DrawRectangleRoundedLines(Rectangle{ PLAYFIELD_X + 6.0f, panelY, PLAYFIELD_WIDTH - 12.0f, panelH }, 0.15f, 4, Fade(SKYBLUE, 0.5f));

    float slotYOffsets[4] = { 10.0f, 6.0f, 4.0f, 8.0f };
    float slotHeights[4] = { 52.0f, 56.0f, 58.0f, 54.0f };
    float slotWidths[4] = { 60.0f, 58.0f, 59.0f, 61.0f };
    float slotXOffsets[4] = { 5.0f, 6.0f, 5.5f, 4.5f };
    float roundnessValues[4] = { 0.35f, 0.5f, 0.25f, 0.4f };

    for (int i = 0; i < LANE_COUNT; ++i) {
        float slotX = LANE_START_X + slotXOffsets[i] + (LANE_WIDTH * i);
        float slotW = slotWidths[i];
        float slotY = panelY + slotYOffsets[i];
        float slotH = slotHeights[i];
        float roundness = roundnessValues[i];
        Rectangle slotRect = { slotX, slotY, slotW, slotH };

        Color slotColor = Color{ 30, 36, 52, 255 };
        if (i < 4 && (s_LanePulseTimers[i] > 0.0f || s_LanePulseTimers[i + 2] > 0.0f)) {
            slotColor = Color{ 0, 220, 255, 255 }; 
            DrawRectangle(static_cast<int>(slotX), static_cast<int>(JUDGMENT_LINE_Y - 12.0f), static_cast<int>(slotW), 20, Fade(SKYBLUE, 0.8f));
        }

        DrawRectangleRounded(slotRect, roundness, 4, slotColor);
        DrawRectangleRoundedLines(slotRect, roundness, 4, Fade(SKYBLUE, 0.7f));
    }

    if (s_IsRecording) {
        std::string statusText = s_IsAutoRecording ? "자동 레코딩 중 (지연 시간: 40ms) - U 키로 정지" : editorTexts[4];
        DrawTextEx(s_SuitFont, statusText.c_str(), Vector2{ PLAYFIELD_X + 10.0f, 20.0f }, 12.0f, 1.0f, RED);
    } else {
        DrawTextEx(s_SuitFont, editorTexts[5].c_str(), Vector2{ PLAYFIELD_X + 10.0f, 20.0f }, 11.0f, 1.0f, GREEN);
    }

    std::string chartSpeedDisplay = editorTexts[6] + (s_IsSpeedInputActive ? s_SpeedInputString + "_" : std::to_string((int)s_ScrollSpeed));
    DrawTextEx(s_SuitFont, chartSpeedDisplay.c_str(), Vector2{ leftVisualizerX + 380.0f, leftVisualizerY + 625.0f }, 14.0f, 1.0f, SKYBLUE);

    ChartSave::DrawChartSystemUI();

    if (s_IsMusicSelectOpen) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        Vector2 mousePos = GetMousePosition();

        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.75f));

        float boxWidth = 450.f;
        float boxHeight = 60.f + s_MusicFileList.size() * 45.f;
        if (boxHeight < 100.f) boxHeight = 100.f;
        if (boxHeight > 500.f) boxHeight = 500.f;

        Rectangle box = { (float)screenWidth / 2.f - boxWidth / 2.f, (float)screenHeight / 2.f - boxHeight / 2.f, boxWidth, boxHeight };
        DrawRectangleRounded(box, 0.15f, 8, Color{ 18, 22, 35, 250 });
        DrawRectangleRoundedLines(box, 0.15f, 8, SKYBLUE);

        DrawTextEx(s_SuitFont, editorTexts[7].c_str(), Vector2{ box.x + 20.f, box.y + 15.f }, 16.0f, 1.0f, RAYWHITE);

        float startY = box.y + 40.f;
        for (size_t i = 0; i < s_MusicFileList.size(); ++i) {
            Rectangle itemBox = { box.x + 20.f, startY + (float)(i * 45.f), boxWidth - 40.f, 35.f };
            bool isHovered = CheckCollisionPointRec(mousePos, itemBox);
            bool isSelected = (s_SelectedMusicIndex == (int)i);

            Color itemBg = isSelected ? Color{ 0, 180, 220, 255 } : (isHovered ? Color{ 40, 50, 75, 255 } : Color{ 28, 34, 48, 255 });
            
            DrawRectangleRounded(itemBox, 0.3f, 4, itemBg);
            DrawRectangleRoundedLines(itemBox, 0.3f, 4, Fade(SKYBLUE, 0.5f));

            DrawTextEx(s_SuitFont, s_MusicFileList[i].c_str(), Vector2{ itemBox.x + 15.f, itemBox.y + 8.f }, 16.0f, 1.0f, RAYWHITE);
        }
    }
}

void ChartEditor::Release() {
    if (s_IsFontLoaded && s_SuitFont.texture.id != 0) {
        UnloadFont(s_SuitFont);
        s_IsFontLoaded = false;
    }
    if (s_FftDsp) {
        FMOD_DSP_Release(s_FftDsp);
        s_FftDsp = nullptr;
    }
    if (m_MusicPlayer) {
        delete m_MusicPlayer;
        m_MusicPlayer = nullptr;
    }
    if (m_AudioManager) {
        delete m_AudioManager;
        m_AudioManager = nullptr;
    }
}