#include "chart_editor.h"
#include "chart_save.h"
#include "editor_play.h"
#include "../music_execute/music1_on.cpp"
#include "../AudioManager/audio_manager.h"
#include <cmath>
#include <vector>
#include <string>

static EditorPlay s_EditorPlay;
static bool s_IsRecording = false;
static bool s_IsTestPlaying = false;
static float s_CurrentPitch = 1.0f;

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
static const float PIXELS_PER_SECOND = 200.0f;

ChartEditor::ChartEditor() 
    : scrollOffset(0.0f) {
    m_MusicPlayer = new MusicExecute::MusicPlayer1();
    m_AudioManager = new AudioManager();
}

ChartEditor::~ChartEditor() {
    Release();
}

void ChartEditor::Init() {
#ifdef NDEBUG
    return;
#else
    scrollOffset = 0.0f;
    s_IsRecording = false;
    s_IsTestPlaying = false;
    s_CurrentPitch = 1.0f;
    notes.clear();

    if (m_AudioManager->Init()) {
        m_AudioManager->Update();
        
        if (m_MusicPlayer->Initialize(*m_AudioManager)) {
            m_MusicPlayer->Play(*m_AudioManager, 0);
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
    }
#endif
}

void ChartEditor::HandleInput() {
#ifdef NDEBUG
    return;
#else
    m_AudioManager->Update();

    static std::string loadedMusicPath = "music/A_Night_Without_Visible_Stars.ogg";
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
        "music/A_Night_Without_Visible_Stars.ogg", 
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
        m_MusicPlayer->Update(GetFrameTime());

        float currentSec = (float)m_MusicPlayer->GetCurrentPositionMs() / 1000.0f;
        scrollOffset = currentSec * PIXELS_PER_SECOND;
        float worldY = scrollOffset;

        if (IsKeyPressed(KEY_D)) {
            notes.push_back({ 0, worldY });
        }
        if (IsKeyPressed(KEY_F)) {
            notes.push_back({ 1, worldY });
        }
        if (IsKeyPressed(KEY_J)) {
            notes.push_back({ 2, worldY });
        }
        if (IsKeyPressed(KEY_K)) {
            notes.push_back({ 3, worldY });
        }

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_U)) {
            s_IsRecording = false;
            m_MusicPlayer->Stop();
            return;
        }
        return;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        notes.clear();
        scrollOffset = 0.0f;
        m_MusicPlayer->Play(*m_AudioManager, 0);
        m_MusicPlayer->PlayImmediate();
        if (m_MusicPlayer) {
            m_MusicPlayer->SetPitch(s_CurrentPitch);
        }
        s_IsRecording = true;
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        Texture2D dummyTex = { 0 };
        s_EditorPlay.Init(currentSaveData, dummyTex);
        
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
#endif
}

void ChartEditor::Render() {
#ifdef NDEBUG
    return;
#else
    if (s_IsTestPlaying) {
        s_EditorPlay.Draw();
        return;
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
        DrawText("RECORDING... (Press SPACE to Stop)", (int)PLAYFIELD_X + 10, 20, 16, RED);
    } else {
        DrawText("SPACE: Record | ENTER: Test Play | '0': Save", (int)PLAYFIELD_X + 5, 20, 12, GREEN);
    }

    std::string speedText = "Speed/Pitch: " + std::to_string((int)(s_CurrentPitch * 100.0f)) + "%";
    DrawText(speedText.c_str(), 1000, 90, 20, WHITE);

    ChartSave::DrawChartSystemUI();
#endif
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