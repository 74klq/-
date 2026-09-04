#include "chart_editor.h"
#include "chart_save.h"
#include "editor_play.h"
#include <cmath>
#include <vector>
#include <string>

static EditorPlay s_EditorPlay;
static bool s_IsTestPlaying = false;

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
}

ChartEditor::~ChartEditor() {
    Release();
}

void ChartEditor::Init() {
#ifdef NDEBUG
    return;
#else
    scrollOffset = 0.0f;
    s_IsTestPlaying = false;
    notes.clear();

    if (m_AudioManager.Init()) {
        if (m_MusicPlayer.Initialize(m_AudioManager)) {
            m_MusicPlayer.Play(m_AudioManager, 0);
        }
    }
#endif
}

void ChartEditor::HandleInput() {
#ifdef NDEBUG
    return;
#else
    m_MusicPlayer.Update(GetFrameTime());

    if (s_IsTestPlaying) {
        if (IsKeyPressed(KEY_U)) {
            s_IsTestPlaying = false;
            return;
        }
        s_EditorPlay.Update(s_IsTestPlaying);
        return;
    }

    if (IsKeyPressed(KEY_ENTER)) {
        s_IsTestPlaying = true;
        std::vector<SaveNoteData> saveData;
        for (const auto& note : notes) {
            saveData.push_back({ note.lane, note.posX });
        }
        Texture2D dummyTex = { 0 };
        s_EditorPlay.Init(saveData, dummyTex);
        return;
    }

    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f) {
        scrollOffset -= wheelMove * 40.0f;
        if (scrollOffset > 0.0f) {
            scrollOffset = 0.0f;
        }
    }

    if (IsKeyPressed(KEY_BACKSPACE)) {
        if (!notes.empty()) {
            notes.pop_back();
        }
    }

    if (IsKeyPressed(KEY_F)) {
        std::vector<SaveNoteData> saveData;
        for (const auto& note : notes) {
            saveData.push_back({ note.lane, note.posX });
        }
        ChartSave::SaveToJSON("my_map.json", saveData);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        if (mousePos.x >= LANE_START_X && mousePos.x <= LANE_START_X + LANE_AREA_WIDTH &&
            mousePos.y >= 0 && mousePos.y <= JUDGMENT_LINE_Y) {
            
            int clickedLane = -1;
            for (int i = 0; i < LANE_COUNT; ++i) {
                float laneLeft = LANE_START_X + (LANE_WIDTH * i);
                if (mousePos.x >= laneLeft && mousePos.x < laneLeft + LANE_WIDTH) {
                    clickedLane = i;
                    break;
                }
            }

            if (clickedLane != -1) {
                float worldY = mousePos.y + scrollOffset;
                notes.push_back({ clickedLane, worldY });
            }
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
    float startWorldY = floorf(scrollOffset / gridSize) * gridSize;
    float endWorldY = scrollOffset + JUDGMENT_LINE_Y;

    for (float worldY = startWorldY; worldY <= endWorldY; worldY += gridSize) {
        float screenY = worldY - scrollOffset;
        if (screenY >= 0.0f && screenY <= JUDGMENT_LINE_Y) {
            bool isBarLine = (int(worldY) % int(gridSize * 4)) == 0;
            float alpha = isBarLine ? 0.25f : 0.08f;
            DrawLine((int)LANE_START_X, (int)screenY, (int)(LANE_START_X + LANE_AREA_WIDTH), (int)screenY, Fade(WHITE, alpha));
        }
    }

    DrawLineEx({ LANE_START_X, JUDGMENT_LINE_Y }, { LANE_START_X + LANE_AREA_WIDTH, JUDGMENT_LINE_Y }, 2.0f, WHITE);

    for (const auto& note : notes) {
        float screenNoteY = note.posX - scrollOffset;
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

    float msgTimer = m_MusicPlayer.GetMsgTimer();
    if (msgTimer > 0.0f) {
        float alpha = (msgTimer > 0.5f) ? 1.0f : (msgTimer / 0.5f);
        int speedPercent = (int)(m_MusicPlayer.GetSpeedRatio() * 100.0f + 0.5f);
        std::string speedText = "Speed: " + std::to_string(speedPercent) + "%";
        DrawText(speedText.c_str(), 1000, 50, 22, Fade(WHITE, alpha));
    }
#endif
}

void ChartEditor::Release() {
}