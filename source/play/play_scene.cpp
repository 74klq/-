#include "play_scene.h"
#include "note.h"
#include "../editing/editor_scene.h"
#include "../vfx/hit_effect.h"
#include <cmath>
#include <vector>
#include <string>

static const float PLAYFIELD_X = 410.0f;
static const float PLAYFIELD_Y = 0.0f;
static const float PLAYFIELD_WIDTH = 320.0f;
static const float PLAYFIELD_HEIGHT = 720.0f;

static const int   LANE_COUNT = 4;
static const float LANE_WIDTH = 70.0f;
static const float LANE_AREA_WIDTH = LANE_COUNT * LANE_WIDTH;
static const float LANE_START_X = PLAYFIELD_X + (PLAYFIELD_WIDTH - LANE_AREA_WIDTH) / 2.0f;

static const float LANE_X_COORDS[4] = {
    LANE_START_X + LANE_WIDTH * 0.5f,
    LANE_START_X + LANE_WIDTH * 1.5f,
    LANE_START_X + LANE_WIDTH * 2.5f,
    LANE_START_X + LANE_WIDTH * 3.5f
};

static std::vector<Note> s_Notes;
static float s_SpawnTimer = 0.0f;
static int s_Combo = 0;
static bool  s_ShowJudgment = false;
static float s_JudgmentTimer = 0.0f;
static const char* s_CurrentJudgment = "PERFECT";

static bool  s_IsEditorMode = false;
static EditorScene s_EditorScene;

static float s_ClockAngle = 0.0f;

static Font s_ComboFont = { 0 };
static Font s_SuitFont = { 0 };

static float s_JudgmentLinePulse = 0.0f;
static float s_JudgmentAnimTimer = 0.0f;
static float s_ComboAnimTimer = 0.0f;
static int s_LastCombo = 0;

static void DrawBackground() {
    DrawRectangle(0, 0, 1280, 720, Color{ 8, 8, 8, 255 });
    DrawRectangle(0, 0, (int)PLAYFIELD_X, 720, Color{ 4, 4, 4, 245 });
    DrawRectangle((int)(PLAYFIELD_X + PLAYFIELD_WIDTH), 0, (int)(1280 - (PLAYFIELD_X + PLAYFIELD_WIDTH)), 720, Color{ 4, 4, 4, 245 });
}

static void DrawPlayfield() {
    DrawRectangleRounded(Rectangle{ PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT }, 0.03f, 4, Color{ 14, 14, 14, 250 });
    DrawRectangleRoundedLines(Rectangle{ PLAYFIELD_X, PLAYFIELD_Y, PLAYFIELD_WIDTH, PLAYFIELD_HEIGHT }, 0.03f, 4, Fade(WHITE, 0.3f));
}

static void DrawLanes(const bool pressedStates[4], float judgmentLineY) {
    for (int i = 0; i < LANE_COUNT; ++i) {
        float laneX = LANE_START_X + (LANE_WIDTH * i);

        DrawRectangle((int)laneX, 0, (int)LANE_WIDTH, (int)judgmentLineY, Fade(WHITE, i % 2 == 0 ? 0.015f : 0.003f));

        if (i > 0) {
            DrawLine((int)laneX, 0, (int)laneX, (int)judgmentLineY, Fade(WHITE, 0.08f));
        }

        if (pressedStates[i]) {
            DrawRectangle((int)laneX, 0, (int)LANE_WIDTH, (int)judgmentLineY, Fade(WHITE, 0.08f));
        }
    }
}

static void DrawNotes() {
    for (auto& note : s_Notes) {
        if (note.active) {
            float noteY = note.y;
            DrawRectangleRounded(Rectangle{ note.x - 29.0f, noteY - 7.0f, 58.0f, 14.0f }, 0.4f, 4, Fade(WHITE, 0.25f));
            DrawRectangleRounded(Rectangle{ note.x - 26.0f, noteY - 5.0f, 52.0f, 10.0f }, 0.3f, 4, WHITE);
            DrawRectangleRounded(Rectangle{ note.x - 22.0f, noteY - 2.0f, 44.0f, 4.0f }, 0.3f, 4, Color{ 25, 25, 25, 255 });
        }
    }
}

static void DrawJudgmentLine(float judgmentLineY) {
    float glowThickness = 2.0f + s_JudgmentLinePulse * 4.0f;
    DrawRectangleRec(Rectangle{ PLAYFIELD_X + 10.0f, judgmentLineY - glowThickness / 2.0f, PLAYFIELD_WIDTH - 20.0f, glowThickness }, Fade(WHITE, 0.25f + s_JudgmentLinePulse * 0.4f));
    DrawLineEx({ LANE_START_X, judgmentLineY }, { LANE_START_X + LANE_AREA_WIDTH, judgmentLineY }, 2.0f, WHITE);
}

static void DrawJudgmentText() {
    if (s_ShowJudgment && s_SuitFont.texture.id != 0) {
        float scale = 1.0f + (s_JudgmentAnimTimer > 0.0f ? s_JudgmentAnimTimer * 0.25f : 0.0f);
        float fontSize = 24.0f * scale;
        Vector2 jSize = MeasureTextEx(s_SuitFont, s_CurrentJudgment, fontSize, 1.0f);
        
        Color col = GOLD;
        std::string jStr(s_CurrentJudgment);
        if (jStr == "PERFECT") col = GOLD;
        else if (jStr == "GREAT") col = GREEN;
        else if (jStr == "GOOD") col = LIGHTGRAY;
        else if (jStr == "MISS") col = RED;

        DrawTextEx(s_SuitFont, s_CurrentJudgment, { PLAYFIELD_X + (PLAYFIELD_WIDTH - jSize.x) / 2.0f, 515.0f }, fontSize, 1.0f, col);
    }
}

static void DrawInputPanel(const bool pressedStates[4]) {
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

        if (pressedStates[i]) {
            slotRect.y += 2.0f;
            slotRect.height -= 2.0f;
            DrawRectangleRounded(slotRect, roundness, 4, WHITE);
            DrawRectangleRoundedLines(slotRect, roundness, 4, BLACK);
        } else {
            DrawRectangleRounded(slotRect, roundness, 4, Color{ 30, 30, 30, 255 });
            DrawRectangleRoundedLines(slotRect, roundness, 4, Fade(WHITE, 0.4f));
        }
    }
}

static void DrawInputFeedback() {
    float centerX = PLAYFIELD_X + PLAYFIELD_WIDTH / 2.0f;
    float centerY = 672.0f;
    float radius = 18.0f;

    DrawCircleLines((int)centerX, (int)centerY, radius, Fade(WHITE, 0.6f));
    DrawCircle((int)centerX, (int)centerY, radius - 2.0f, Color{ 15, 15, 15, 255 });

    for (int i = 0; i < 12; ++i) {
        float angle = i * 30.0f * (PI / 180.0f);
        float innerR = (i % 3 == 0) ? radius - 5.0f : radius - 3.5f;
        float outerR = radius - 2.0f;
        float x1 = centerX + cosf(angle) * innerR;
        float y1 = centerY + sinf(angle) * innerR;
        float x2 = centerX + cosf(angle) * outerR;
        float y2 = centerY + sinf(angle) * outerR;
        DrawLineEx({ x1, y1 }, { x2, y2 }, (i % 3 == 0) ? 1.5f : 1.0f, Fade(WHITE, 0.5f));
    }

    DrawLineEx({ centerX, centerY }, { centerX, centerY - 8.0f }, 1.5f, Fade(WHITE, 0.7f));
    DrawLineEx({ centerX, centerY }, { centerX + 8.0f, centerY }, 1.2f, Fade(WHITE, 0.5f));

    float rad = s_ClockAngle * (PI / 180.0f);
    float needleX = centerX + cosf(rad) * (radius - 4.0f);
    float needleY = centerY + sinf(rad) * (radius - 4.0f);
    DrawLineEx({ centerX, centerY }, { needleX, needleY }, 1.0f, RED);

    DrawCircle((int)centerX, (int)centerY, 2.0f, WHITE);
}

static void DrawHUD() {
    if (s_Combo > 1 && s_ComboFont.texture.id != 0) {
        std::string comboStr = std::to_string(s_Combo);
        float scale = 1.0f + (s_ComboAnimTimer > 0.0f ? s_ComboAnimTimer * 0.4f : 0.0f);
        float fontSize = 60.0f * scale;
        Vector2 textSize = MeasureTextEx(s_ComboFont, comboStr.c_str(), fontSize, 2.0f);
        
        DrawTextEx(s_ComboFont, comboStr.c_str(), { PLAYFIELD_X + (PLAYFIELD_WIDTH - textSize.x) / 2.0f, 180.0f }, fontSize, 2.0f, GOLD);
        
        float labelSize = 18.0f;
        Vector2 labelSizeVec = MeasureTextEx(s_ComboFont, "COMBO", labelSize, 2.0f);
        DrawTextEx(s_ComboFont, "COMBO", { PLAYFIELD_X + (PLAYFIELD_WIDTH - labelSizeVec.x) / 2.0f, 235.0f }, labelSize, 2.0f, Fade(WHITE, 0.7f));
    }
}

PlayScene::PlayScene() 
    : m_State(PlaySceneState::SongSelect), m_BackToMenu(false), judgmentLineY(595.0f) {
}

PlayScene::~PlayScene() {
}

void PlayScene::Init() {
    m_State = PlaySceneState::SongSelect;
    m_BackToMenu = false;
    judgmentLineY = 595.0f;
    
    m_SongSelect.Init();

    s_Notes.clear();
    s_SpawnTimer = 0.0f;
    s_Combo = 0;
    s_ShowJudgment = false;
    s_JudgmentTimer = 0.0f;
    s_IsEditorMode = false;
    s_ClockAngle = 0.0f;
    s_JudgmentLinePulse = 0.0f;
    s_JudgmentAnimTimer = 0.0f;
    s_ComboAnimTimer = 0.0f;
    s_LastCombo = 0;
    
    s_ComboFont = LoadFont("fonts/combo_font.ttf");
    s_SuitFont = LoadFont("fonts/SUIT-Medium.ttf");
}

void PlayScene::Update() {
    if (m_State == PlaySceneState::SongSelect) {
        m_SongSelect.Update();

        if (m_SongSelect.IsBackSelected()) {
            m_BackToMenu = true;
        }
        else if (m_SongSelect.IsPlaySelected()) {
            m_State = PlaySceneState::Playing;
        }
    }
    else if (m_State == PlaySceneState::Playing) {
        UpdatePlaying();
    }
}

void PlayScene::UpdatePlaying() {
    if (IsKeyPressed(KEY_P)) {
        s_IsEditorMode = !s_IsEditorMode;
        if (s_IsEditorMode) {
            s_EditorScene.Init();
        }
    }

    if (s_IsEditorMode) {
        if (IsKeyPressed(KEY_ONE)) {
            s_IsEditorMode = false;
            return;
        }
        s_EditorScene.HandleInput();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        m_State = PlaySceneState::SongSelect;
        return;
    }

    float dt = GetFrameTime();
    s_ClockAngle += dt * 180.0f;

    if (s_JudgmentLinePulse > 0.0f) {
        s_JudgmentLinePulse -= dt * 4.0f;
        if (s_JudgmentLinePulse < 0.0f) s_JudgmentLinePulse = 0.0f;
    }

    if (s_JudgmentAnimTimer > 0.0f) {
        s_JudgmentAnimTimer -= dt;
    }

    if (s_ComboAnimTimer > 0.0f) {
        s_ComboAnimTimer -= dt;
    }

    s_SpawnTimer += dt;
    if (s_SpawnTimer >= 0.45f) {
        int lane = GetRandomValue(0, 3);
        s_Notes.push_back(Note(LANE_X_COORDS[lane], -40.0f, 6.0f, lane));
        s_SpawnTimer = 0.0f;
    }

    for (auto& note : s_Notes) {
        note.Update();
    }

    HitEffect::Update();

    for (auto it = s_Notes.begin(); it != s_Notes.end();) {
        if (it->active && it->y > judgmentLineY + 35.0f) {
            it->active = false;
            s_Combo = 0;
            s_LastCombo = 0;
            s_ShowJudgment = true;
            s_JudgmentTimer = 0.3f;
            s_CurrentJudgment = "MISS";
            s_JudgmentAnimTimer = 0.3f;
            it = s_Notes.erase(it);
        } else {
            ++it;
        }
    }

    int inputLane = -1;
    if (IsKeyPressed(KEY_D)) inputLane = 0;
    if (IsKeyPressed(KEY_F)) inputLane = 1;
    if (IsKeyPressed(KEY_J)) inputLane = 2;
    if (IsKeyPressed(KEY_K)) inputLane = 3;

    if (inputLane != -1) {
        bool hitRecorded = false;
        for (auto it = s_Notes.begin(); it != s_Notes.end(); ++it) {
            if (it->active && it->lane == inputLane) {
                float dist = fabsf(it->y - judgmentLineY);
                if (dist <= 35.0f) {
                    it->active = false;
                    HitEffect::Spawn({it->x, judgmentLineY});
                    s_Notes.erase(it);

                    s_ShowJudgment = true;
                    s_JudgmentTimer = 0.4f;
                    s_JudgmentLinePulse = 1.0f;
                    s_JudgmentAnimTimer = 0.3f;

                    if (dist <= 16.0f) {
                        s_CurrentJudgment = "PERFECT";
                        s_Combo++;
                    } else if (dist <= 26.0f) {
                        s_CurrentJudgment = "GREAT";
                        s_Combo = 0;
                        s_LastCombo = 0;
                    } else {
                        s_CurrentJudgment = "GOOD";
                        s_Combo = 0;
                        s_LastCombo = 0;
                    }

                    if (std::string(s_CurrentJudgment) == "PERFECT" && s_Combo != s_LastCombo) {
                        s_ComboAnimTimer = 0.2f;
                        s_LastCombo = s_Combo;
                    }

                    hitRecorded = true;
                    break;
                }
            }
        }
        if (!hitRecorded) {
            s_Combo = 0;
            s_LastCombo = 0;
        }
    }

    if (s_ShowJudgment) {
        s_JudgmentTimer -= dt;
        if (s_JudgmentTimer <= 0.0f) {
            s_ShowJudgment = false;
        }
    }
}

void PlayScene::Draw() {
    if (m_State == PlaySceneState::SongSelect) {
        m_SongSelect.Draw(GetScreenWidth(), GetScreenHeight());
    }
    else if (m_State == PlaySceneState::Playing) {
        DrawPlaying();
    }
}

void PlayScene::DrawPlaying() {
    if (s_IsEditorMode) {
        s_EditorScene.Render();
        return;
    }

    bool isDPressed = IsKeyDown(KEY_D);
    bool isFPressed = IsKeyDown(KEY_F);
    bool isJPressed = IsKeyDown(KEY_J);
    bool isKPressed = IsKeyDown(KEY_K);
    bool pressedStates[4] = { isDPressed, isFPressed, isJPressed, isKPressed };

    DrawBackground();
    DrawPlayfield();
    DrawLanes(pressedStates, judgmentLineY);
    DrawNotes();
    DrawJudgmentLine(judgmentLineY);
    DrawJudgmentText();
    DrawInputPanel(pressedStates);
    DrawInputFeedback();
    HitEffect::Draw();
    DrawHUD();
}

void PlayScene::Unload() {
    if (s_ComboFont.texture.id != 0) UnloadFont(s_ComboFont);
    if (s_SuitFont.texture.id != 0) UnloadFont(s_SuitFont);
    s_EditorScene.Release();
}