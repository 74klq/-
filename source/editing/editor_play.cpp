#include "editor_play.h"
#include <cmath>
#include <vector>
#include <string>

static std::vector<bool> s_NoteActiveStates;
static int s_Combo = 0;
static bool s_ShowJudgment = false;
static float s_JudgmentTimer = 0.0f;
static const char* s_CurrentJudgment = "PERFECT";

static float s_ClockAngle = 0.0f;
static Font s_ComboFont = { 0 };
static Font s_SuitFont = { 0 };

static float s_JudgmentLinePulse = 0.0f;
static float s_JudgmentAnimTimer = 0.0f;
static float s_ComboAnimTimer = 0.0f;
static int s_LastCombo = 0;
static float s_PlayElapsedTime = 0.0f;

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
static const float JUDGMENT_LINE_Y = 595.0f;
static const float NOTE_SPEED = 400.0f;

EditorPlay::EditorPlay() {
}

EditorPlay::~EditorPlay() {
    Unload();
}

void EditorPlay::Init(const std::vector<SaveNoteData>& notes, Texture2D noteTex) {
    playNotes = notes;
    s_NoteActiveStates.assign(notes.size(), true);
    s_Combo = 0;
    s_ShowJudgment = false;
    s_JudgmentTimer = 0.0f;
    s_ClockAngle = 0.0f;
    s_JudgmentLinePulse = 0.0f;
    s_JudgmentAnimTimer = 0.0f;
    s_ComboAnimTimer = 0.0f;
    s_LastCombo = 0;
    s_PlayElapsedTime = 0.0f;

    s_ComboFont = LoadFont("fonts/combo_font.ttf");
    s_SuitFont = LoadFont("fonts/SUIT-Medium.ttf");
}

void EditorPlay::Update(bool& isPlaying) {
    if (IsKeyPressed(KEY_U)) {
        isPlaying = false;
        return;
    }

    float dt = GetFrameTime();
    s_PlayElapsedTime += dt;
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

    float currentWorldY = 0.0f;
    if (s_PlayElapsedTime > 0.0f) {
        currentWorldY = s_PlayElapsedTime * NOTE_SPEED;
    }

    for (size_t i = 0; i < playNotes.size(); ++i) {
        if (i < s_NoteActiveStates.size() && s_NoteActiveStates[i]) {
            float screenNoteY = JUDGMENT_LINE_Y - (playNotes[i].posX - currentWorldY);
            
            if (screenNoteY > JUDGMENT_LINE_Y + 80.0f) {
                s_NoteActiveStates[i] = false;
                s_Combo = 0;
                s_LastCombo = 0;
                s_ShowJudgment = true;
                s_JudgmentTimer = 0.3f;
                s_CurrentJudgment = "MISS";
                s_JudgmentAnimTimer = 0.3f;
            }
        }
    }

    int inputLane = -1;
    if (IsKeyPressed(KEY_D)) inputLane = 0;
    if (IsKeyPressed(KEY_F)) inputLane = 1;
    if (IsKeyPressed(KEY_J)) inputLane = 2;
    if (IsKeyPressed(KEY_K)) inputLane = 3;

    if (inputLane != -1) {
        bool hitRecorded = false;
        for (size_t i = 0; i < playNotes.size(); ++i) {
            if (playNotes[i].lane != inputLane) continue;
            if (i < s_NoteActiveStates.size() && !s_NoteActiveStates[i]) continue;

            float diff = fabsf(playNotes[i].posX - currentWorldY);

            if (diff <= 50.0f) {
                s_NoteActiveStates[i] = false;

                s_ShowJudgment = true;
                s_JudgmentTimer = 0.4f;
                s_JudgmentLinePulse = 1.0f;
                s_JudgmentAnimTimer = 0.3f;

                if (diff <= 15.0f) {
                    s_CurrentJudgment = "PERFECT";
                    s_Combo++;
                } else if (diff <= 35.0f) {
                    s_CurrentJudgment = "GREAT";
                    s_Combo++;
                } else {
                    s_CurrentJudgment = "GOOD";
                    s_Combo++;
                }

                if (s_Combo != s_LastCombo) {
                    s_ComboAnimTimer = 0.2f;
                    s_LastCombo = s_Combo;
                }

                hitRecorded = true;
                break;
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

void EditorPlay::Draw() {
    bool isDPressed = IsKeyDown(KEY_D);
    bool isFPressed = IsKeyDown(KEY_F);
    bool isJPressed = IsKeyDown(KEY_J);
    bool isKPressed = IsKeyDown(KEY_K);
    bool pressedStates[4] = { isDPressed, isFPressed, isJPressed, isKPressed };

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
        if (pressedStates[i]) {
            DrawRectangle((int)laneX, 0, (int)LANE_WIDTH, (int)JUDGMENT_LINE_Y, Fade(WHITE, 0.08f));
        }
    }

    float currentWorldY = 0.0f;
    if (s_PlayElapsedTime > 0.0f) {
        currentWorldY = s_PlayElapsedTime * NOTE_SPEED;
    }

    for (size_t i = 0; i < playNotes.size(); ++i) {
        if (playNotes[i].lane < 0 || playNotes[i].lane >= 4) continue;
        if (i < s_NoteActiveStates.size() && !s_NoteActiveStates[i]) continue;

        float screenNoteY = JUDGMENT_LINE_Y - (playNotes[i].posX - currentWorldY);

        if (screenNoteY >= -40.0f && screenNoteY <= JUDGMENT_LINE_Y + 40.0f) {
            float noteX = LANE_X_COORDS[playNotes[i].lane];
            DrawRectangleRounded(Rectangle{ noteX - 29.0f, screenNoteY - 7.0f, 58.0f, 14.0f }, 0.4f, 4, Fade(WHITE, 0.25f));
            DrawRectangleRounded(Rectangle{ noteX - 26.0f, screenNoteY - 5.0f, 52.0f, 10.0f }, 0.3f, 4, WHITE);
            DrawRectangleRounded(Rectangle{ noteX - 22.0f, screenNoteY - 2.0f, 44.0f, 4.0f }, 0.3f, 4, Color{ 25, 25, 25, 255 });
        }
    }

    float glowThickness = 2.0f + s_JudgmentLinePulse * 4.0f;
    DrawRectangleRec(Rectangle{ PLAYFIELD_X + 10.0f, JUDGMENT_LINE_Y - glowThickness / 2.0f, PLAYFIELD_WIDTH - 20.0f, glowThickness }, Fade(WHITE, 0.25f + s_JudgmentLinePulse * 0.4f));
    DrawLineEx({ LANE_START_X, JUDGMENT_LINE_Y }, { LANE_START_X + LANE_AREA_WIDTH, JUDGMENT_LINE_Y }, 2.0f, WHITE);

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

    if (s_Combo > 1 && s_ComboFont.texture.id != 0) {
        std::string comboStr = std::to_string(s_Combo);
        float scale = 1.0f + (s_ComboAnimTimer > 0.0f ? s_ComboAnimTimer * 0.4f : 0.0f);
        float fontSize = 42.0f * scale;
        Vector2 textSize = MeasureTextEx(s_ComboFont, comboStr.c_str(), fontSize, 2.0f);
        
        DrawTextEx(s_ComboFont, comboStr.c_str(), { PLAYFIELD_X + (PLAYFIELD_WIDTH - textSize.x) / 2.0f, 180.0f }, fontSize, 2.0f, GOLD);
        
        float labelSize = 12.0f;
        Vector2 labelSizeVec = MeasureTextEx(s_ComboFont, "COMBO", labelSize, 2.0f);
        DrawTextEx(s_ComboFont, "COMBO", { PLAYFIELD_X + (PLAYFIELD_WIDTH - labelSizeVec.x) / 2.0f, 230.0f }, labelSize, 2.0f, Fade(WHITE, 0.7f));
    }
}

void EditorPlay::Unload() {
    if (s_ComboFont.texture.id != 0) UnloadFont(s_ComboFont);
    if (s_SuitFont.texture.id != 0) UnloadFont(s_SuitFont);
    playNotes.clear();
    s_NoteActiveStates.clear();
}