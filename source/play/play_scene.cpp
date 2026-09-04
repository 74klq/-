#include "play_scene.h"
#include "note.h"
#include "../editing/editor_scene.h"
#include "../vfx/hit_effect.h"
#include <cmath>
#include <vector>
#include <string>

static std::vector<Note> s_Notes;
static float s_SpawnTimer = 0.0f;
static int s_Combo = 0;
static bool  s_ShowJudgment = false;
static float s_JudgmentTimer = 0.0f;
static const char* s_CurrentJudgment = "PERFECT";

static const float LANE_X_COORDS[4] = { 457.5f, 532.5f, 607.5f, 682.5f };

static bool  s_IsEditorMode = false;
static EditorScene s_EditorScene;

static float s_ClockAngle = 0.0f;

static Font s_ComboFont = { 0 };
static Font s_SuitFont = { 0 };

PlayScene::PlayScene() {
    judgmentLineY = 560.0f;
}

PlayScene::~PlayScene() {
}

void PlayScene::Init() {
    judgmentLineY = 560.0f;
    s_Notes.clear();
    s_SpawnTimer = 0.0f;
    s_Combo = 0;
    s_ShowJudgment = false;
    s_JudgmentTimer = 0.0f;
    s_IsEditorMode = false;
    s_ClockAngle = 0.0f;
    
    s_ComboFont = LoadFont("fonts/combo_font.ttf");
    s_SuitFont = LoadFont("fonts/SUIT-Medium.ttf");

    s_EditorScene.Init();
}

void PlayScene::Update() {
    if (IsKeyPressed(KEY_P)) {
        s_IsEditorMode = !s_IsEditorMode;
    }

    if (s_IsEditorMode) {
        if (IsKeyPressed(KEY_SPACE)) {
            s_IsEditorMode = false;
            return;
        }
        s_EditorScene.HandleInput();
        return;
    }

    float dt = GetFrameTime();
    s_ClockAngle += dt * 180.0f;

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

    int inputLane = -1;
    if (IsKeyPressed(KEY_D) || IsKeyDown(KEY_D)) inputLane = 0;
    if (IsKeyPressed(KEY_F) || IsKeyDown(KEY_F)) inputLane = 1;
    if (IsKeyPressed(KEY_J) || IsKeyDown(KEY_J)) inputLane = 2;
    if (IsKeyPressed(KEY_K) || IsKeyDown(KEY_K)) inputLane = 3;

    if (inputLane != -1) {
        for (auto& note : s_Notes) {
            if (note.active && note.lane == inputLane) {
                float dist = fabsf(note.y - judgmentLineY);
                if (dist < 60.0f) {
                    note.active = false;
                    s_Combo++;
                    s_ShowJudgment = true;
                    s_JudgmentTimer = 0.4f;
                    Color effectColor = GOLD;
                    if (dist < 20.0f) {
                        s_CurrentJudgment = "PERFECT";
                        effectColor = GOLD;
                    } else if (dist < 40.0f) {
                        s_CurrentJudgment = "GREAT";
                        effectColor = GREEN;
                    } else {
                        s_CurrentJudgment = "GOOD";
                        effectColor = SKYBLUE;
                    }
                    HitEffect::Spawn({note.x, judgmentLineY}, effectColor);
                    break;
                }
            }
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
    if (s_IsEditorMode) {
        s_EditorScene.Render();
        return;
    }

    for (int i = 0; i < 4; ++i) {
        float laneX = 420.0f + (75.0f * i);
        DrawRectangle((int)laneX, 0, 75, 720, Fade(SKYBLUE, 0.02f));
        DrawLine((int)laneX, 0, (int)laneX, 720, Fade(WHITE, 0.1f));
    }
    DrawLine(720, 0, 720, 720, Fade(WHITE, 0.2f));

    bool isDPressed = IsKeyDown(KEY_D);
    bool isFPressed = IsKeyDown(KEY_F);
    bool isJPressed = IsKeyDown(KEY_J);
    bool isKPressed = IsKeyDown(KEY_K);
    bool pressedStates[4] = { isDPressed, isFPressed, isJPressed, isKPressed };

    for (int i = 0; i < 4; ++i) {
        if (pressedStates[i]) {
            float laneX = 420.0f + (75.0f * i);
            DrawRectangle((int)laneX, 0, 75, 652, Fade(WHITE, 0.08f));
        }
    }

    for (int i = 0; i < 3; ++i) {
        DrawLine(420 - i, 0, 420 - i, 720, Fade(WHITE, 0.8f));
        DrawLine(720 + i, 0, 720 + i, 720, Fade(WHITE, 0.8f));
        DrawLine(420, 720 + i, 720, 720 + i, Fade(WHITE, 0.8f));
    }

    DrawLineEx({420.0f, 595.0f}, {720.0f, 595.0f}, 3.0f, RAYWHITE);
    DrawLineEx({420.0f, 595.0f}, {720.0f, 595.0f}, 1.0f, YELLOW);

    float slotY = 652.0f;
    float slotW = 75.0f;
    float slotH = 68.0f;
    float slotXCoords[4] = { 420.0f, 495.0f, 570.0f, 645.0f };

    for (int i = 0; i < 4; ++i) {
        if (pressedStates[i]) {
            DrawRectangle((int)slotXCoords[i], (int)slotY, (int)slotW, (int)slotH, WHITE);
            DrawRectangleLines((int)slotXCoords[i], (int)slotY, (int)slotW, (int)slotH, BLACK);
        } else {
            DrawRectangleLines((int)slotXCoords[i], (int)slotY, (int)slotW, (int)slotH, Fade(WHITE, 0.6f));
            DrawRectangle((int)slotXCoords[i], (int)slotY, (int)slotW, (int)slotH, Fade(WHITE, 0.05f));
        }
    }

    float centerX = 570.0f;
    float centerY = 686.0f;
    float radius = 16.0f;
    DrawCircleLines((int)centerX, (int)centerY, radius, WHITE);
    DrawCircle((int)centerX, (int)centerY, radius - 4.0f, BLACK);

    float rad = s_ClockAngle * (PI / 180.0f);
    float needleX = centerX + cosf(rad) * (radius - 5.0f);
    float needleY = centerY + sinf(rad) * (radius - 5.0f);
    DrawLine((int)centerX, (int)centerY, (int)needleX, (int)needleY, YELLOW);

    for (auto& note : s_Notes) {
        if (note.active) {
            DrawRectangle((int)(note.x - 32.5f), (int)(note.y - 8.0f), 65, 16, RAYWHITE);
            DrawRectangle((int)(note.x - 28.5f), (int)(note.y - 4.0f), 57, 8, BLACK);
        }
    }

    HitEffect::Draw();

    if (s_Combo > 1 && s_ComboFont.texture.id != 0) {
        std::string comboStr = std::to_string(s_Combo);
        float fontSize = 44.0f;
        Vector2 textSize = MeasureTextEx(s_ComboFont, comboStr.c_str(), fontSize, 2.0f);
        DrawTextEx(s_ComboFont, comboStr.c_str(), { 570.0f - textSize.x / 2.0f, 210.0f }, fontSize, 2.0f, YELLOW);
        
        float labelSize = 16.0f;
        Vector2 labelSizeVec = MeasureTextEx(s_ComboFont, "COMBO", labelSize, 2.0f);
        DrawTextEx(s_ComboFont, "COMBO", { 570.0f - labelSizeVec.x / 2.0f, 260.0f }, labelSize, 2.0f, LIGHTGRAY);
    }

    if (s_ShowJudgment && s_SuitFont.texture.id != 0) {
        float fontSize = 26.0f;
        Vector2 jSize = MeasureTextEx(s_SuitFont, s_CurrentJudgment, fontSize, 1.0f);
        Color col = (std::string(s_CurrentJudgment) == "PERFECT") ? GOLD : (std::string(s_CurrentJudgment) == "GREAT" ? GREEN : SKYBLUE);
        DrawTextEx(s_SuitFont, s_CurrentJudgment, { 570.0f - jSize.x / 2.0f, 330.0f }, fontSize, 1.0f, col);
    }
}

void PlayScene::Unload() {
    if (s_ComboFont.texture.id != 0) UnloadFont(s_ComboFont);
    if (s_SuitFont.texture.id != 0) UnloadFont(s_SuitFont);
    s_EditorScene.Release();
}