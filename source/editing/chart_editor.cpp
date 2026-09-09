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
    /* 0 */ "AUDIO SPECTRUM & PITCH VISUALIZER",
    /* 1 */ "에디팅 프로그램",
    /* 2 */ "Press '7' to play music & start live spectrum analysis",
    /* 3 */ "Current Pitch: ",
    /* 4 */ "RECORDING (Latency: 40ms) - U to Stop",
    /* 5 */ "'7': Record | 'ENTER': Test | '0': Save | 'T': Spd | 'Q': Music",
    /* 6 */ "speed = ",
    /* 7 */ "Select Music (Click & Enter):"
};

static EditorPlay s_EditorPlay;
static bool s_IsRecording = false;
static bool s_IsTestPlaying = false;
static float s_CurrentPitch = 1.0f;
static float s_ScrollSpeed = 200.0f;
static bool s_IsSpeedInputActive = false;
static std::string s_SpeedInputString = "";

static float s_InputLatency = 0.040f; 
static float s_EditorBPM = 130.0f;     

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
    s_IsTestPlaying = false;
    s_CurrentPitch = 1.0f;
    s_ScrollSpeed = 200.0f;
    s_IsSpeedInputActive = false;
    s_SpeedInputString.clear();
    s_IsMusicSelectOpen = false;
    s_CurrentMusicPath = "";
    s_IsFftInitialized = false;
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

    if (IsKeyPressed(KEY_Q) && !ChartSave::IsPopupOpen() && !s_IsTestPlaying && !s_IsRecording) {
        s_IsMusicSelectOpen = !s_IsMusicSelectOpen;
        if (s_IsMusicSelectOpen) {
            s_MusicFileList.clear();
            std::string dirPath = "music";
            if (std::filesystem::exists(dirPath)) {
                for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
                    if (entry.path().extension() == ".ogg" || entry.path().extension() == ".mp3" || entry.path().extension() == ".wav") {
                        s_MusicFileList.push_back("music/" + entry.path().filename().string());
                    }
                }
            }
            s_SelectedMusicIndex = 0;
        }
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
        for (size_t i = 0; i < s_MusicFileList.size(); ++i) {
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
        if (s_CurrentPitch < 0.25f) {
            s_CurrentPitch = 0.25f;
        }
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
    }

    if (IsKeyPressed(KEY_FOUR)) {
        s_CurrentPitch += 0.05f;
        if (s_CurrentPitch > 1.0f) {
            s_CurrentPitch = 1.0f;
        }
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
    }

    if (s_IsTestPlaying) {
        if (IsKeyPressed(KEY_U)) {
            s_IsTestPlaying = false;
            m_MusicPlayer->Stop(); 
            return;
        }
        s_EditorPlay.Update(s_IsTestPlaying);
        return;
    }

    if (s_IsRecording) {
        if (!m_MusicPlayer) return;
        float currentSec = (float)m_MusicPlayer->GetCurrentPositionMs() / 1000.0f;
        
        float exactInputTime = currentSec - s_InputLatency;
        if (exactInputTime < 0.0f) exactInputTime = 0.0f;
        
        scrollOffset = currentSec * s_ScrollSpeed;
        float worldY = exactInputTime * s_ScrollSpeed;

        int recordKeys[] = { KEY_Z, KEY_X, KEY_C, KEY_B, KEY_N, KEY_M };
        static float s_LastRecordedTime = -1.0f;

        int triggeredKeyIndex = -1;
        for (int idx = 0; idx < 6; ++idx) {
            if (IsKeyPressed(recordKeys[idx])) {
                triggeredKeyIndex = idx;
                s_LanePulseTimers[idx] = 0.15f; 
                break;
            }
        }

        if (triggeredKeyIndex != -1) {
            if (s_LastRecordedTime < 0.0f || (currentSec - s_LastRecordedTime >= 0.12f)) {
                s_LastRecordedTime = currentSec;

                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<int> laneDist(0, LANE_COUNT - 1);
                int chosenLane = laneDist(gen);

                float beatIntervalSec = 60.0f / s_EditorBPM;
                float subBeatIntervalSec = beatIntervalSec / 4.0f; 
                
                float nearestBeatTime = roundf(exactInputTime / subBeatIntervalSec) * subBeatIntervalSec;
                float errorSec = exactInputTime - nearestBeatTime;
                float errorMs = fabsf(errorSec) * 1000.0f;

                Color noteColor = WHITE;
                if (errorMs <= 30.0f) {
                    noteColor = GREEN; 
                } else if (errorMs <= 60.0f) {
                    noteColor = YELLOW; 
                } else {
                    noteColor = RED; 
                }

                s_EditorNotes.push_back({ chosenLane, worldY, errorMs, noteColor });
                
                notes.clear();
                for (const auto& n : s_EditorNotes) {
                    notes.push_back({ n.lane, n.posX });
                }
            }
        }

        if (IsKeyPressed(KEY_U)) {
            s_IsRecording = false;
            m_MusicPlayer->Stop();
            return;
        }
        return;
    }

    if (IsKeyPressed(KEY_SEVEN)) {
        if (s_CurrentMusicPath.empty()) return;
        s_EditorNotes.clear();
        notes.clear();
        scrollOffset = 0.0f;
        m_MusicPlayer->Stop();
        m_MusicPlayer->Play(*m_AudioManager, 0);
        m_MusicPlayer->PlayImmediate();
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
        s_IsRecording = true;
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

    std::string pitchStr = editorTexts[3] + std::to_string((int)(s_CurrentPitch * 100.0f)) + "% (Use 3/4)";
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
        DrawTextEx(s_SuitFont, editorTexts[4].c_str(), Vector2{ PLAYFIELD_X + 10.0f, 20.0f }, 12.0f, 1.0f, RED);
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