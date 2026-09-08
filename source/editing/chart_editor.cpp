#include "chart_editor.h"
#include "chart_save.h"
#include "editor_play.h"
#include "../music_execute/music1_on.cpp"
#include "../AudioManager/audio_manager.h"
#include <cmath>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <algorithm>

static EditorPlay s_EditorPlay;
static bool s_IsRecording = false;
static bool s_IsTestPlaying = false;
static float s_CurrentPitch = 1.0f;
static float s_ScrollSpeed = 200.0f;
static bool s_IsSpeedInputActive = false;
static std::string s_SpeedInputString = "";

static bool s_IsMusicSelectOpen = false;
static std::vector<std::string> s_MusicFileList;
static int s_SelectedMusicIndex = 0;
static std::string s_CurrentMusicPath = "";

static const float PLAYFIELD_X = 410.0f;
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
    notes.clear();

    if (m_AudioManager->Init()) {
        m_AudioManager->Update();
    }
}

void ChartEditor::HandleInput() {
    m_AudioManager->Update();

    if (m_MusicPlayer) {
        m_MusicPlayer->Update(GetFrameTime());
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
        notes.clear();
        for (const auto& n : loadedNotes) {
            notes.push_back({ n.lane, n.posX });
        }
        fileLoaded = false;
    }

    std::vector<SaveNoteData> currentSaveData;
    for (size_t i = 0; i < notes.size(); ++i) {
        currentSaveData.push_back({ notes[i].lane, notes[i].posX });
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
        scrollOffset = currentSec * s_ScrollSpeed;
        float worldY = scrollOffset;

        int recordKeys[] = { KEY_Z, KEY_X, KEY_C, KEY_B, KEY_N, KEY_M };
        static float s_LastRecordedTime = -1.0f;
        static int s_NextForcedLane = 0; // 누락 방지를 위한 순차 순환 인덱스

        // 어떤 키든 입력되는지 검사
        int triggeredKeyIndex = -1;
        for (int idx = 0; idx < 6; ++idx) {
            if (IsKeyPressed(recordKeys[idx])) {
                triggeredKeyIndex = idx;
                break;
            }
        }

        if (triggeredKeyIndex != -1) {
            // 난이도 살짝 낮춤: 최소 노트 간격을 0.18초로 넉넉하게 조정
            if (s_LastRecordedTime < 0.0f || (currentSec - s_LastRecordedTime >= 0.18f)) {
                s_LastRecordedTime = currentSec;

                // 누락 방지: 사용자가 어떤 키를 눌렀든, 4개 레인(0~3)에 전부 골고루 들어가도록 순차 배분
                int chosenLane = s_NextForcedLane;
                s_NextForcedLane = (s_NextForcedLane + 1) % LANE_COUNT;

                notes.push_back({ chosenLane, worldY });
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
        notes.clear();
        scrollOffset = 0.0f;
        m_MusicPlayer->Stop();
        m_MusicPlayer->Play(*m_AudioManager, 0);
        m_MusicPlayer->PlayImmediate();
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
        s_IsRecording = true;
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
        if (!notes.empty()) {
            notes.pop_back();
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
    }

    DrawRectangle(0, 0, 1280, 720, Color{ 8, 8, 8, 255 });
    DrawRectangle(0, 0, (int)PLAYFIELD_X, 720, Color{ 4, 4, 4, 245 });
    DrawRectangle((int)(PLAYFIELD_X + PLAYFIELD_WIDTH), 0, (int)(1280 - (PLAYFIELD_X + PLAYFIELD_WIDTH)), 720, Color{ 4, 4, 4, 245 });

    DrawRectangleRounded(Rectangle{ PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT }, 0.03f, 4, Color{ 14, 14, 14, 250 });
    DrawRectangleRoundedLines(Rectangle{ PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT }, 0.03f, 4, Fade(WHITE, 0.3f));

    for (int i = 0; i < LANE_COUNT; ++i) {
        float laneX = LANE_START_X + (LANE_WIDTH * i);
        DrawRectangle((int)laneX, 0, (int)LANE_WIDTH, (int)JUDGMENT_LINE_Y, Fade(WHITE, i % 2 == 0 ? 0.015f : 0.003f));
        if (i > 0) {
            DrawLine((int)laneX, 0, (int)laneX, (int)JUDGMENT_LINE_Y, Fade(WHITE, 0.08f));
        }
    }

    float gridSize = 50.0f;
    float startWorldY = scrollOffset - JUDGMENT_LINE_Y;
    if (startWorldY < 0.0f) startWorldY = 0.0f;
    float endWorldY = scrollOffset + JUDGMENT_LINE_Y;

    float firstGridY = ceilf(startWorldY / gridSize) * gridSize;
    for (float worldY = firstGridY; worldY <= endWorldY; worldY += gridSize) {
        float screenY = JUDGMENT_LINE_Y - (worldY - scrollOffset);
        if (screenY >= 0.0f && screenY <= JUDGMENT_LINE_Y) {
            bool isBarLine = (int(worldY) % int(gridSize * 4)) == 0;
            float alpha = isBarLine ? 0.25f : 0.08f;
            DrawLine((int)LANE_START_X, (int)screenY, (int)(LANE_START_X + LANE_AREA_WIDTH), (int)screenY, Fade(WHITE, alpha));
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
                3.0f, Fade(RAYWHITE, 0.4f)
            );
            DrawLineEx(
                { LANE_START_X, syncLineScreenY }, 
                { LANE_START_X + LANE_AREA_WIDTH, syncLineScreenY }, 
                1.5f, RAYWHITE
            );
        }
    }

    DrawLineEx({ LANE_START_X, JUDGMENT_LINE_Y }, { LANE_START_X + LANE_AREA_WIDTH, JUDGMENT_LINE_Y }, 2.0f, WHITE);

    for (const auto& note : notes) {
        float screenNoteY = JUDGMENT_LINE_Y - (note.posX - scrollOffset);
        if (screenNoteY >= -30.0f && screenNoteY <= JUDGMENT_LINE_Y + 30.0f) {
            float noteX = LANE_X_COORDS[note.lane];
            DrawRectangleRounded(Rectangle{ noteX - 26.0f, screenNoteY - 5.0f, 52.0f, 10.0f }, 0.3f, 4, WHITE);
            DrawRectangleRounded(Rectangle{ noteX - 22.0f, screenNoteY - 2.0f, 44.0f, 4.0f }, 0.3f, 4, Color{ 25, 25, 25, 255 });
        }
    }

    float panelY = 636.0f;
    float panelH = 72.0f;
    DrawRectangleRounded(Rectangle{ PLAYFIELD_X + 6.0f, panelY, PLAYFIELD_WIDTH - 12.0f, panelH }, 0.15f, 4, Color{ 16, 16, 16, 255 });
    DrawRectangleRoundedLines(Rectangle{ PLAYFIELD_X + 6.0f, panelY, PLAYFIELD_WIDTH - 12.0f, panelH }, 0.15f, 4, Fade(WHITE, 0.3f));

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

        DrawRectangleRounded(slotRect, roundness, 4, Color{ 30, 30, 30, 255 });
        DrawRectangleRoundedLines(slotRect, roundness, 4, Fade(WHITE, 0.4f));
    }

    if (s_IsRecording) {
        DrawText("RECORDING (Balanced & Even Mode) - Press U to Stop", (int)PLAYFIELD_X + 10, 20, 14, RED);
    } else {
        DrawText("'7': Record | ENTER: Test Play | '0': Save | 'T': Speed | 'Q': Music", (int)PLAYFIELD_X + 5, 20, 12, GREEN);
    }

    std::string speedText = "Speed/Pitch: " + std::to_string((int)(s_CurrentPitch * 100.0f)) + "%";
    DrawText(speedText.c_str(), 1000, 90, 20, WHITE);

    std::string chartSpeedDisplay = "chart speed = " + (s_IsSpeedInputActive ? s_SpeedInputString + "_" : std::to_string((int)s_ScrollSpeed));
    DrawText(chartSpeedDisplay.c_str(), 20, 360, 20, WHITE);

    ChartSave::DrawChartSystemUI();

    if (s_IsMusicSelectOpen) {
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        Vector2 mousePos = GetMousePosition();

        DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));

        float boxWidth = 450.f;
        float boxHeight = 60.f + s_MusicFileList.size() * 45.f;
        if (boxHeight < 100.f) boxHeight = 100.f;
        if (boxHeight > 500.f) boxHeight = 500.f;

        Rectangle box = { (float)screenWidth / 2.f - boxWidth / 2.f, (float)screenHeight / 2.f - boxHeight / 2.f, boxWidth, boxHeight };
        DrawRectangleRounded(box, 0.15f, 8, RAYWHITE);
        DrawRectangleRoundedLines(box, 0.15f, 8, LIGHTGRAY);

        DrawText("Select Music (Click & Enter):", (int)box.x + 20, (int)box.y + 15, 16, DARKGRAY);

        float startY = box.y + 40.f;
        for (size_t i = 0; i < s_MusicFileList.size(); ++i) {
            Rectangle itemBox = { box.x + 20.f, startY + (float)(i * 45.f), boxWidth - 40.f, 35.f };
            bool isHovered = CheckCollisionPointRec(mousePos, itemBox);
            bool isSelected = (s_SelectedMusicIndex == (int)i);

            Color itemBg = isSelected ? Color{ 200, 220, 255, 255 } : (isHovered ? Color{ 230, 240, 255, 255 } : WHITE);
            
            DrawRectangleRounded(itemBox, 0.3f, 4, itemBg);
            DrawRectangleRoundedLines(itemBox, 0.3f, 4, LIGHTGRAY);

            DrawText(s_MusicFileList[i].c_str(), (int)itemBox.x + 15, (int)itemBox.y + 8, 16, DARKGRAY);
        }
    }
}

void ChartEditor::Release() {
    if (m_MusicPlayer) {
        delete m_MusicPlayer;
        m_MusicPlayer = nullptr;
    }
    if (m_AudioManager) {
        delete m_AudioManager;
        m_AudioManager = nullptr;
    }
}