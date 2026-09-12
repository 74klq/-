#include "play_scene.h"
#include "note.h"
#include "../editing/chart_editor.h"
#include "../editing/chart_save.h"
#include "../vfx/hit_effect.h"
#include "../AudioManager/audio_manager.h"
#include "../music_execute/music1_on.cpp"
#include <cmath>
#include <vector>
#include <string>

static const float PLAYFIELD_X = 410.0f;
static const float PLAYFIELD_Y = 0.0f;
static const float PLAYFIELD_WIDTH = 320.0f;
static const float PLAYFIELD_HEIGHT = 720.0f;

static const int   LANE_COUNT = 4;
static const float LANE_WIDTH = 80.0f;
static const float LANE_AREA_WIDTH = LANE_COUNT * LANE_WIDTH;
static const float LANE_START_X = PLAYFIELD_X + (PLAYFIELD_WIDTH - LANE_AREA_WIDTH) / 2.0f;

static const float LANE_X_COORDS[4] = {
    LANE_START_X + LANE_WIDTH * 0.5f,
    LANE_START_X + LANE_WIDTH * 1.5f,
    LANE_START_X + LANE_WIDTH * 2.5f,
    LANE_START_X + LANE_WIDTH * 3.5f
};

static AudioManager s_AudioManager;
static MusicExecute::MusicPlayer1* s_MusicPlayer = nullptr;

static std::vector<Note> s_Notes;
struct PlayableNote {
    float timeSec;
    int lane;
    bool active;
};
static std::vector<PlayableNote> s_PlayableNotes;
static float s_SongTimer = 0.0f;

static float s_SpawnTimer = 0.0f;
static int s_Combo = 0;
static bool  s_ShowJudgment = false;
static float s_JudgmentTimer = 0.0f;
static const char* s_CurrentJudgment = "PERFECT";

static bool  s_IsEditorMode = false;
static ChartEditor s_ChartEditor;

static Font s_ComboFont = { 0 };
static Font s_SuitFont = { 0 };

static float s_JudgmentLinePulse = 0.0f;
static float s_JudgmentAnimTimer = 0.0f;
static float s_ComboAnimTimer = 0.0f;
static int s_LastCombo = 0;

static float s_NoteScrollSpeed = 200.0f;

static void DrawBackground() {
    DrawRectangleGradientV(0, 0, 1280, 720, Color{ 8, 8, 12, 255 }, Color{ 1, 1, 3, 255 });

    for (int i = 0; i < 90; ++i) {
        float fx = fmodf(i * 137.0f, 1280.0f);
        float fy = fmodf(i * 79.0f, 430.0f) + 8.0f;
        float twinkle = 0.35f + 0.35f * sinf(GetTime() * (0.45f + (i % 5) * 0.08f) + i);
        unsigned char alpha = (unsigned char)(35.0f + twinkle * 55.0f);
        DrawCircle((int)fx, (int)fy, (i % 7 == 0) ? 1.6f : 1.0f, Color{ 235, 235, 240, alpha });
    }

    for (int y = 40; y < 720; y += 40) {
        DrawLine(0, y, 1280, y, Color{ 255, 255, 255, 9 });
    }

    for (int x = 0; x < 1280; x += 80) {
        DrawLine(x, 0, x, 720, Color{ 255, 255, 255, 7 });
    }

    float pulse = 0.5f + 0.5f * sinf(GetTime() * 0.7f);
    Color horizon = Fade(Color{ 255, 255, 255, 255 }, 0.04f + pulse * 0.025f);
    DrawRectangle(0, 430, 1280, 1, horizon);
}

static void DrawMechanicalFrame() {
    const int left = (int)PLAYFIELD_X;
    const int right = (int)(PLAYFIELD_X + PLAYFIELD_WIDTH);

    DrawRectangle(left - 12, 0, 12, 720, Color{ 22, 22, 25, 255 });
    DrawRectangle(right, 0, 12, 720, Color{ 22, 22, 25, 255 });

    DrawRectangle(left - 6, 0, 3, 720, Color{ 145, 145, 150, 220 });
    DrawRectangle(right + 3, 0, 3, 720, Color{ 145, 145, 150, 220 });

    DrawRectangle(left - 2, 0, 2, 720, Color{ 235, 235, 235, 210 });
    DrawRectangle(right, 0, 2, 720, Color{ 235, 235, 235, 210 });

    for (int i = 0; i < 6; ++i) {
        float y = 95.0f + i * 92.0f;
        DrawRectangle(left - 10, (int)y, 8, 3, Color{ 95, 95, 100, 220 });
        DrawRectangle(right + 2, (int)y, 8, 3, Color{ 95, 95, 100, 220 });
    }

    DrawTriangle(
        { (float)left - 6.0f, 430.0f },
        { (float)left - 62.0f, 548.0f },
        { (float)left - 6.0f, 590.0f },
        Color{ 215, 215, 218, 255 }
    );
    DrawTriangle(
        { (float)right + 6.0f, 430.0f },
        { (float)right + 62.0f, 548.0f },
        { (float)right + 6.0f, 590.0f },
        Color{ 215, 215, 218, 255 }
    );

    DrawTriangleLines(
        { (float)left - 6.0f, 430.0f },
        { (float)left - 62.0f, 548.0f },
        { (float)left - 6.0f, 590.0f },
        Color{ 70, 70, 75, 255 }
    );
    DrawTriangleLines(
        { (float)right + 6.0f, 430.0f },
        { (float)right + 62.0f, 548.0f },
        { (float)right + 6.0f, 590.0f },
        Color{ 70, 70, 75, 255 }
    );

    DrawRectangle(left - 4, 0, 4, 395, Color{ 12, 12, 15, 255 });
    DrawRectangle(right, 0, 4, 395, Color{ 12, 12, 15, 255 });

    DrawRectangleGradientV(left - 2, 392, 4, 90, Color{ 250, 250, 250, 245 }, Color{ 70, 70, 75, 255 });
    DrawRectangleGradientV(right - 2, 392, 4, 90, Color{ 250, 250, 250, 245 }, Color{ 70, 70, 75, 255 });
}

static void DrawPlayfield() {
    DrawRectangle((int)PLAYFIELD_X, (int)PLAYFIELD_Y, (int)PLAYFIELD_WIDTH, (int)PLAYFIELD_HEIGHT, Color{ 7, 8, 10, 255 });

    DrawRectangleGradientH((int)PLAYFIELD_X, 0, 32, 720, Color{ 0, 0, 0, 235 }, Color{ 0, 0, 0, 0 });
    DrawRectangleGradientH((int)(PLAYFIELD_X + PLAYFIELD_WIDTH - 32), 0, 32, 720, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 235 });

    float scanPos = fmodf(GetTime() * 120.0f, PLAYFIELD_HEIGHT);
    DrawRectangle((int)PLAYFIELD_X + 1, (int)scanPos, (int)PLAYFIELD_WIDTH - 2, 1, Color{ 255, 255, 255, 18 });

    DrawRectangle((int)PLAYFIELD_X, 0, (int)PLAYFIELD_WIDTH, 2, Color{ 230, 230, 232, 120 });
    DrawRectangle((int)PLAYFIELD_X, 718, (int)PLAYFIELD_WIDTH, 2, Color{ 55, 55, 60, 255 });

    DrawMechanicalFrame();
}

static void DrawLaneDecorations(float judgmentLineY) {
    for (int i = 0; i < LANE_COUNT; ++i) {
        float laneX = LANE_START_X + (LANE_WIDTH * i);

        if (i > 0) {
            DrawRectangle((int)laneX - 2, 0, 4, (int)judgmentLineY, Color{ 0, 0, 0, 190 });
            DrawRectangle((int)laneX - 1, 0, 2, (int)judgmentLineY, Color{ 155, 155, 160, 125 });

            for (int y = 20; y < (int)judgmentLineY; y += 68) {
                DrawRectangle((int)laneX - 1, y, 2, 14, Color{ 210, 210, 215, 28 });
            }
        }

        if (i == 0) {
            DrawRectangle((int)laneX, 0, 1, (int)judgmentLineY, Color{ 70, 70, 75, 65 });
        }
    }
}

static void DrawLanes(const bool pressedStates[4], float judgmentLineY) {
    DrawLaneDecorations(judgmentLineY);

    for (int i = 0; i < LANE_COUNT; ++i) {
        float laneX = LANE_START_X + (LANE_WIDTH * i);

        if (pressedStates[i]) {
            DrawRectangleGradientV(
                (int)laneX,
                0,
                (int)LANE_WIDTH,
                (int)judgmentLineY,
                Color{ 255, 255, 255, 0 },
                Color{ 255, 255, 255, 38 }
            );

            DrawRectangle(
                (int)laneX + 2,
                (int)judgmentLineY - 42,
                (int)LANE_WIDTH - 4,
                1,
                Color{ 255, 255, 255, 65 }
            );
        }
    }
}

static void DrawNotes(float judgmentLineY) {
    for (const auto& pNote : s_PlayableNotes) {
        if (!pNote.active) continue;

        float timeRemaining = pNote.timeSec - s_SongTimer;
        float noteY = judgmentLineY - (timeRemaining * s_NoteScrollSpeed);

        if (noteY >= -50.0f && noteY <= 770.0f) {
            float nX = LANE_X_COORDS[pNote.lane];
            float nW = 68.0f;
            float nH = 18.0f;
            float corner = 0.18f;
            int segs = 5;

            DrawRectangleRounded(
                Rectangle{ nX - nW / 2.0f + 2.0f, noteY - nH / 2.0f + 6.0f, nW, nH },
                corner, segs, Color{ 0, 0, 0, 220 }
            );

            DrawRectangleRounded(
                Rectangle{ nX - nW / 2.0f, noteY - nH / 2.0f, nW, nH },
                corner, segs, Color{ 228, 228, 230, 255 }
            );

            DrawRectangleRounded(
                Rectangle{ nX - nW / 2.0f + 2.0f, noteY - nH / 2.0f + 2.0f, nW - 4.0f, nH / 2.0f - 1.0f },
                corner, segs, Color{ 255, 255, 255, 255 }
            );

            DrawRectangle(
                (int)(nX - nW / 2.0f + 12.0f),
                (int)(noteY - 1.0f),
                (int)(nW - 24.0f),
                2,
                Color{ 35, 35, 38, 255 }
            );

            DrawRectangleRoundedLines(
                Rectangle{ nX - nW / 2.0f, noteY - nH / 2.0f, nW, nH },
                corner, segs, Color{ 20, 20, 24, 255 }
            );
        }
    }
}

static void DrawJudgmentLine(float judgmentLineY) {
    float pulseAlpha = 0.68f + s_JudgmentLinePulse * 0.32f;
    Color outer = Fade(Color{ 230, 230, 232, 255 }, 0.28f * pulseAlpha);
    Color mid = Fade(Color{ 255, 255, 255, 255 }, 0.65f * pulseAlpha);
    Color core = Fade(WHITE, pulseAlpha);

    DrawRectangle((int)LANE_START_X - 8, (int)judgmentLineY - 7, (int)LANE_AREA_WIDTH + 16, 14, outer);
    DrawRectangle((int)LANE_START_X - 3, (int)judgmentLineY - 3, (int)LANE_AREA_WIDTH + 6, 6, mid);
    DrawRectangle((int)LANE_START_X, (int)judgmentLineY - 1, (int)LANE_AREA_WIDTH, 2, core);

    DrawRectangle((int)LANE_START_X - 18, (int)judgmentLineY - 1, 10, 2, Color{ 255, 255, 255, 145 });
    DrawRectangle((int)LANE_START_X + (int)LANE_AREA_WIDTH + 8, (int)judgmentLineY - 1, 10, 2, Color{ 255, 255, 255, 145 });
}

static void DrawJudgmentText() {
    if (s_ShowJudgment && s_SuitFont.texture.id != 0) {
        float scale = 1.0f + (s_JudgmentAnimTimer > 0.0f ? s_JudgmentAnimTimer * 0.3f : 0.0f);
        float fontSize = 30.0f * scale;

        std::string rawJudgment(s_CurrentJudgment);
        std::string jStr = "- " + rawJudgment + " -";
        Vector2 jSize = MeasureTextEx(s_SuitFont, jStr.c_str(), fontSize, 3.0f);

        Color col = Color{ 245, 245, 245, 255 };
        if (rawJudgment == "PERFECT") col = Color{ 255, 255, 255, 255 };
        else if (rawJudgment == "GREAT") col = Color{ 205, 205, 210, 255 };
        else if (rawJudgment == "GOOD") col = Color{ 155, 155, 160, 255 };
        else if (rawJudgment == "MISS") col = Color{ 90, 90, 95, 255 };

        float basePositionY = 280.0f;
        if (s_Combo > 1 && s_ComboFont.texture.id != 0) {
            float comboScale = 1.0f + (s_ComboAnimTimer > 0.0f ? s_ComboAnimTimer * 0.4f : 0.0f);
            float comboFontSize = 80.0f * comboScale;
            float comboTextY = 200.0f - (s_ComboAnimTimer * 15.0f);
            basePositionY = comboTextY + comboFontSize + 10.0f;
        }

        float textX = PLAYFIELD_X + (PLAYFIELD_WIDTH - jSize.x) / 2.0f;
        float textY = basePositionY - (s_JudgmentAnimTimer * 8.0f);

        DrawTextEx(s_SuitFont, jStr.c_str(), { textX + 3.0f, textY + 3.0f }, fontSize, 3.0f, Color{ 0, 0, 0, 230 });
        DrawTextEx(s_SuitFont, jStr.c_str(), { textX, textY }, fontSize, 3.0f, col);
    }
}

static void DrawComboHUD() {
    if (s_Combo <= 0 || s_ComboFont.texture.id == 0 || s_SuitFont.texture.id == 0) return;

    float pulse = s_ComboAnimTimer > 0.0f ? s_ComboAnimTimer * 0.45f : 0.0f;
    float comboSize = 72.0f + pulse * 16.0f;
    std::string comboText = std::to_string(s_Combo);
    Vector2 comboSizeVec = MeasureTextEx(s_ComboFont, comboText.c_str(), comboSize, 1.0f);
    float centerX = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;
    float comboX = centerX - comboSizeVec.x * 0.5f;
    float comboY = 112.0f - pulse * 8.0f;

    DrawRectangle((int)(centerX - 118.0f), (int)comboY - 10, 236, 1, Color{ 130, 130, 135, 100 });
    DrawRectangle((int)(centerX - 82.0f), (int)comboY - 5, 164, 1, Color{ 220, 220, 224, 70 });
    DrawRectangle((int)(centerX - 118.0f), (int)comboY + 8, 34, 1, Color{ 220, 220, 224, 130 });
    DrawRectangle((int)(centerX + 84.0f), (int)comboY + 8, 34, 1, Color{ 220, 220, 224, 130 });

    DrawTextEx(s_SuitFont, "COMBO", { centerX - 31.0f, comboY - 31.0f }, 17.0f, 2.0f, Color{ 170, 170, 176, 220 });

    DrawTextEx(s_ComboFont, comboText.c_str(), { comboX + 4.0f, comboY + 4.0f }, comboSize, 1.0f, Color{ 255, 255, 255, 75 });
    DrawTextEx(s_ComboFont, comboText.c_str(), { comboX + 2.0f, comboY + 2.0f }, comboSize, 1.0f, Color{ 10, 10, 12, 255 });
    DrawTextEx(s_ComboFont, comboText.c_str(), { comboX, comboY }, comboSize, 1.0f, WHITE);

    float lineY = comboY + comboSizeVec.y + 5.0f;
    DrawRectangle((int)(centerX - 92.0f), (int)lineY, 48, 2, Color{ 210, 210, 214, 150 });
    DrawRectangle((int)(centerX - 39.0f), (int)lineY, 78, 1, Color{ 255, 255, 255, 85 });
    DrawRectangle((int)(centerX + 44.0f), (int)lineY, 48, 2, Color{ 210, 210, 214, 150 });

    if (s_ComboAnimTimer > 0.0f) {
        float burst = 18.0f + (0.2f - s_ComboAnimTimer) * 70.0f;
        unsigned char alpha = (unsigned char)(80.0f * (s_ComboAnimTimer / 0.2f));
        DrawCircleLines((int)centerX, (int)(comboY + comboSizeVec.y * 0.52f), burst, Color{ 255, 255, 255, alpha });
    }
}

static void DrawInputFeedbackFlash(const bool pressedStates[4], float judgmentLineY) {
    for (int i = 0; i < LANE_COUNT; ++i) {
        if (!pressedStates[i]) continue;

        float laneX = LANE_START_X + i * LANE_WIDTH;
        float pulse = 0.55f + 0.45f * sinf(GetTime() * 12.0f + i * 0.65f);
        unsigned char alpha = (unsigned char)(65.0f + pulse * 90.0f);

        DrawRectangleGradientV(
            (int)laneX + 4,
            24,
            (int)LANE_WIDTH - 8,
            (int)judgmentLineY - 32,
            Color{ 255, 255, 255, 0 },
            Color{ 255, 255, 255, alpha }
        );

        DrawRectangle((int)laneX + 7, (int)judgmentLineY - 17, (int)LANE_WIDTH - 14, 3, Color{ 255, 255, 255, alpha });
        DrawRectangle((int)laneX + 12, (int)judgmentLineY + 7, (int)LANE_WIDTH - 24, 2, Color{ 255, 255, 255, (unsigned char)(alpha * 0.7f) });
    }
}

static void DrawInputPanel(const bool pressedStates[4]) {
    const float panelY = 568.0f;
    const float panelBottom = 720.0f;
    const float panelH = panelBottom - panelY;
    const float innerY = panelY + 8.0f;
    const float buttonH = panelH - 16.0f;
    const float gap = 5.0f;
    const float totalW = PLAYFIELD_WIDTH - 18.0f;
    const float buttonW = (totalW - gap * 3.0f) / 4.0f;

    DrawRectangle((int)PLAYFIELD_X, (int)panelY, (int)PLAYFIELD_WIDTH, (int)panelH, Color{ 7, 7, 9, 255 });
    DrawRectangle((int)PLAYFIELD_X, (int)panelY, (int)PLAYFIELD_WIDTH, 3, Color{ 245, 245, 247, 235 });
    DrawRectangle((int)PLAYFIELD_X, (int)panelY + 3, (int)PLAYFIELD_WIDTH, 2, Color{ 65, 65, 70, 255 });

    for (int i = 0; i < LANE_COUNT; ++i) {
        float x = PLAYFIELD_X + 9.0f + i * (buttonW + gap);
        float cx = x + buttonW * 0.5f;
        const bool pressed = pressedStates[i];
        const char* keyText = (i == 0) ? "D" : (i == 1) ? "F" : (i == 2) ? "J" : "K";

        Color outer = pressed ? Color{ 238, 238, 241, 255 } : Color{ 82, 82, 88, 255 };
        Color faceTop = pressed ? Color{ 250, 250, 251, 255 } : Color{ 53, 53, 58, 255 };
        Color faceBottom = pressed ? Color{ 125, 125, 130, 255 } : Color{ 13, 13, 16, 255 };
        Color textColor = pressed ? Color{ 12, 12, 14, 255 } : Color{ 246, 246, 248, 255 };

        DrawRectangle((int)x, (int)innerY, (int)buttonW, (int)buttonH, Color{ 0, 0, 0, 220 });
        DrawRectangleLines((int)x, (int)innerY, (int)buttonW, (int)buttonH, outer);
        DrawRectangleLines((int)x + 2, (int)innerY + 2, (int)buttonW - 4, (int)buttonH - 4, Color{ 35, 35, 40, 255 });

        DrawRectangleGradientV(
            (int)x + 5,
            (int)innerY + 5,
            (int)buttonW - 10,
            (int)buttonH - 10,
            faceTop,
            faceBottom
        );

        DrawRectangle((int)x + 6, (int)innerY + 6, (int)buttonW - 12, 2, pressed ? Color{ 255, 255, 255, 220 } : Color{ 120, 120, 125, 180 });
        DrawRectangle((int)x + 6, (int)innerY + (int)buttonH - 8, (int)buttonW - 12, 2, pressed ? Color{ 30, 30, 34, 180 } : Color{ 0, 0, 0, 220 });

        if (!pressed) {
            DrawRectangle((int)x + 9, (int)innerY + 11, 3, (int)buttonH - 22, Color{ 160, 160, 165, 70 });
            DrawRectangle((int)x + (int)buttonW - 12, (int)innerY + 11, 3, (int)buttonH - 22, Color{ 0, 0, 0, 150 });
        } else {
            float pulse = 0.55f + 0.45f * sinf(GetTime() * 7.0f);
            unsigned char a = (unsigned char)(95.0f + pulse * 80.0f);
            DrawRectangle((int)x + 7, (int)innerY + 7, (int)buttonW - 14, 3, Color{ 255, 255, 255, a });
        }

        float keyFont = 31.0f;
        Vector2 keySize = MeasureTextEx(s_SuitFont, keyText, keyFont, 1.5f);
        float keyX = cx - keySize.x * 0.5f;
        float keyY = innerY + buttonH * 0.5f - keySize.y * 0.52f;

        DrawRectangleRounded(
            Rectangle{ cx - 21.0f, innerY + buttonH * 0.5f - 27.0f, 42.0f, 54.0f },
            0.10f, 6,
            pressed ? Color{ 235, 235, 238, 255 } : Color{ 26, 26, 30, 255 }
        );
        DrawRectangleRoundedLines(
            Rectangle{ cx - 21.0f, innerY + buttonH * 0.5f - 27.0f, 42.0f, 54.0f },
            0.10f, 6,
            pressed ? Color{ 25, 25, 28, 255 } : Color{ 155, 155, 160, 185 }
        );

        DrawTextEx(s_SuitFont, keyText, { keyX + 2.0f, keyY + 2.0f }, keyFont, 1.5f, Color{ 0, 0, 0, 210 });
        DrawTextEx(s_SuitFont, keyText, { keyX, keyY }, keyFont, 1.5f, textColor);

        float labelY = innerY + 11.0f;
        const char* laneLabel = (i == 0) ? "L-01" : (i == 1) ? "L-02" : (i == 2) ? "L-03" : "L-04";
        DrawTextEx(s_SuitFont, laneLabel, { x + 10.0f, labelY }, 9.0f, 1.0f, Color{ 175, 175, 180, 220 });

        DrawRectangle((int)(x + 10.0f), (int)(innerY + buttonH - 15.0f), (int)(buttonW - 20.0f), 1, Color{ 120, 120, 125, 90 });
        DrawRectangle((int)(x + 10.0f), (int)(innerY + buttonH - 12.0f), 12, 2, pressed ? Color{ 255, 255, 255, 220 } : Color{ 95, 95, 100, 130 });
        DrawRectangle((int)(x + buttonW - 22.0f), (int)(innerY + buttonH - 12.0f), 12, 2, pressed ? Color{ 255, 255, 255, 220 } : Color{ 95, 95, 100, 130 });

        DrawCircle((int)(x + 9.0f), (int)(innerY + 9.0f), 2.0f, Color{ 178, 178, 182, 170 });
        DrawCircle((int)(x + buttonW - 9.0f), (int)(innerY + 9.0f), 2.0f, Color{ 178, 178, 182, 170 });
        DrawCircle((int)(x + 9.0f), (int)(innerY + buttonH - 9.0f), 2.0f, Color{ 70, 70, 75, 255 });
        DrawCircle((int)(x + buttonW - 9.0f), (int)(innerY + buttonH - 9.0f), 2.0f, Color{ 70, 70, 75, 255 });

        if (pressed) {
            DrawRectangle((int)x + 12, (int)innerY + 6, (int)buttonW - 24, 4, Color{ 255, 255, 255, 235 });
            DrawRectangle((int)x + 12, (int)innerY + (int)buttonH - 12, (int)buttonW - 24, 3, Color{ 255, 255, 255, 185 });
            DrawRectangle((int)x + 3, (int)innerY + 3, (int)buttonW - 6, (int)buttonH - 6, Color{ 255, 255, 255, 34 });
            DrawTextEx(s_SuitFont, "HIT", { x + 10.0f, innerY + buttonH - 28.0f }, 10.0f, 1.0f, Color{ 20, 20, 22, 210 });
        }

        if (i < LANE_COUNT - 1) {
            float gx = x + buttonW + gap * 0.5f;
            DrawRectangle((int)gx - 1, (int)innerY + 14, 2, (int)buttonH - 28, Color{ 220, 220, 224, 38 });
        }
    }

    DrawRectangle((int)PLAYFIELD_X + 8, 714, (int)PLAYFIELD_WIDTH - 16, 1, Color{ 135, 135, 140, 120 });
}

static void DrawPlaySceneSideMarkers() {
    const float left = PLAYFIELD_X - 42.0f;
    const float right = PLAYFIELD_X + PLAYFIELD_WIDTH + 42.0f;

    DrawRectangleRounded(
        Rectangle{ left - 20.0f, 92.0f, 28.0f, 48.0f },
        0.18f, 6, Color{ 15, 15, 18, 235 }
    );
    DrawRectangleRounded(
        Rectangle{ right - 8.0f, 92.0f, 28.0f, 48.0f },
        0.18f, 6, Color{ 15, 15, 18, 235 }
    );

    DrawRectangle(left - 8.0f, 110.0f, 10.0f, 2, Color{ 220, 220, 224, 180 });
    DrawRectangle(right - 2.0f, 110.0f, 10.0f, 2, Color{ 220, 220, 224, 180 });

    DrawRectangle(left - 3.0f, 166.0f, 6.0f, 6.0f, Color{ 185, 185, 190, 120 });
    DrawRectangle(right - 3.0f, 166.0f, 6.0f, 6.0f, Color{ 185, 185, 190, 120 });
}

static void DrawUpperTechnicalHUD() {
    float left = PLAYFIELD_X - 17.0f;
    float right = PLAYFIELD_X + PLAYFIELD_WIDTH + 17.0f;

    DrawRectangle((int)left, 18, (int)(right - left), 2, Color{ 200, 200, 205, 100 });
    DrawRectangle((int)left, 22, 36, 1, Color{ 255, 255, 255, 165 });
    DrawRectangle((int)(right - 36.0f), 22, 36, 1, Color{ 255, 255, 255, 165 });

    DrawRectangle((int)left, 34, 86, 22, Color{ 11, 11, 14, 228 });
    DrawRectangleLines((int)left, 34, 86, 22, Color{ 170, 170, 176, 110 });
    DrawTextEx(s_SuitFont, "INPUT MATRIX", { left + 8.0f, 39.0f }, 9.0f, 1.0f, Color{ 178, 178, 184, 220 });

    DrawRectangle((int)(right - 86.0f), 34, 86, 22, Color{ 11, 11, 14, 228 });
    DrawRectangleLines((int)(right - 86.0f), 34, 86, 22, Color{ 170, 170, 176, 110 });
    DrawTextEx(s_SuitFont, "SYNC / 4L", { right - 78.0f, 39.0f }, 9.0f, 1.0f, Color{ 178, 178, 184, 220 });

    for (int i = 0; i < 7; ++i) {
        float x = PLAYFIELD_X + 10.0f + i * 48.0f;
        DrawRectangle((int)x, 62, 24, 2, Color{ 110, 110, 116, 80 });
        DrawRectangle((int)(x + 27.0f), 62, 5, 2, Color{ 210, 210, 214, 120 });
    }
}

PlayScene::PlayScene() 
    : m_State(PlaySceneState::SongSelect), m_BackToMenu(false), judgmentLineY(595.0f) {
    s_MusicPlayer = new MusicExecute::MusicPlayer1();
}

PlayScene::~PlayScene() {
    if (s_MusicPlayer) {
        delete s_MusicPlayer;
        s_MusicPlayer = nullptr;
    }
}

void PlayScene::Init() {
    m_State = PlaySceneState::SongSelect;
    m_BackToMenu = false;
    judgmentLineY = 595.0f;
    
    s_AudioManager.Init();
    if (s_MusicPlayer) {
        s_MusicPlayer->Initialize(s_AudioManager);
    }
    m_SongSelect.Init();

    s_Notes.clear();
    s_PlayableNotes.clear();
    s_SongTimer = 0.0f;
    s_SpawnTimer = 0.0f;
    s_Combo = 0;
    s_ShowJudgment = false;
    s_JudgmentTimer = 0.0f;
    s_IsEditorMode = false;
    s_JudgmentLinePulse = 0.0f;
    s_JudgmentAnimTimer = 0.0f;
    s_ComboAnimTimer = 0.0f;
    s_LastCombo = 0;
    s_NoteScrollSpeed = 200.0f; 
    
    s_ComboFont = LoadFont("fonts/combo_font.ttf");
    s_SuitFont = LoadFont("fonts/SUIT-Medium.ttf");
}

void PlayScene::Update() {
   if (IsKeyPressed(KEY_P)) {
        s_IsEditorMode = true;
        m_State = PlaySceneState::Playing; 
        s_ChartEditor.Init();
        return;
    }

    s_AudioManager.Update();

    if (s_MusicPlayer) {
        s_MusicPlayer->Update(GetFrameTime());
    }

    if (m_State == PlaySceneState::SongSelect) {
        m_SongSelect.Update();

        if (m_SongSelect.IsBackSelected()) {
            m_BackToMenu = true;
        }
        else if (m_SongSelect.IsEditorSelected()) {
            s_IsEditorMode = true;
            m_State = PlaySceneState::Playing; 
            s_ChartEditor.Init();
        }
        else if (m_SongSelect.IsPlaySelected()) {
            m_State = PlaySceneState::Playing;
            
            s_Notes.clear();
            s_PlayableNotes.clear();
            s_SongTimer = 0.0f;

            const SongData& curSong = m_SongSelect.GetCurrentSong();
            
            std::string jsonFileName = "G.json";
            if (curSong.title == "별이 보이지 않는 밤") {
                jsonFileName = "G.json";
            } else if (curSong.title == "Kaleidoscope") {
                jsonFileName = "Kaleidoscope.json";
            } else if (curSong.title == "Timeline") {
                jsonFileName = "Timeline.json";
            } else if (curSong.title == "R") {
                jsonFileName = "R.json";
            }

            std::string outMusicPath;
            float loadedSpeed = 200.0f; 
            std::vector<SaveNoteData> loadedNotes;
            
            if (ChartSave::LoadFromJSON(jsonFileName.c_str(), outMusicPath, loadedSpeed, loadedNotes)) {
                s_NoteScrollSpeed = (loadedSpeed > 1.0f) ? loadedSpeed : 200.0f; 

                for (size_t i = 0; i < loadedNotes.size(); ++i) {
                    const auto& saveNote = loadedNotes[i];
                    PlayableNote pNote;
                    pNote.timeSec = saveNote.posX / s_NoteScrollSpeed; 
                    pNote.lane = saveNote.lane;
                    pNote.active = true;
                    s_PlayableNotes.push_back(pNote);
                }
            }

            if (!outMusicPath.empty()) {
                if (s_MusicPlayer) {
                    s_MusicPlayer->Stop();
                    s_MusicPlayer->Play(s_AudioManager, 0);
                    s_MusicPlayer->PlayImmediate();
                    s_MusicPlayer->SetPitch(1.0f);
                }
            }
        }
    }
    else if (m_State == PlaySceneState::Playing) {
        UpdatePlaying();
    }
}

void PlayScene::UpdatePlaying() {
    if (s_IsEditorMode) {
        if (IsKeyPressed(KEY_P)) {
            s_IsEditorMode = false;
            m_State = PlaySceneState::SongSelect;
            m_BackToMenu = true;
            
            if (s_MusicPlayer) {
                s_MusicPlayer->Stop();
            }
            return;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            s_IsEditorMode = false;
            m_State = PlaySceneState::SongSelect;
            return;
        }
        s_ChartEditor.HandleInput();
        return;
    }

    float dt = GetFrameTime();
    if (s_MusicPlayer) {
        s_SongTimer = (float)s_MusicPlayer->GetCurrentPositionMs() / 1000.0f;
    } else {
        s_SongTimer += dt; 
    }

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

    for (auto& pNote : s_PlayableNotes) {
        if (pNote.active) {
            float timeDiff = s_SongTimer - pNote.timeSec;
            if (timeDiff > 0.3f) { 
                pNote.active = false;
                s_Combo = 0;
                s_LastCombo = 0;
                s_ShowJudgment = true;
                s_JudgmentTimer = 0.3f;
                s_CurrentJudgment = "MISS";
                s_JudgmentAnimTimer = 0.3f;
            }
        }
    }

    HitEffect::Update();

    int inputLane = -1;
    if (IsKeyPressed(KEY_D)) inputLane = 0;
    if (IsKeyPressed(KEY_F)) inputLane = 1;
    if (IsKeyPressed(KEY_J)) inputLane = 2;
    if (IsKeyPressed(KEY_K)) inputLane = 3;

    if (inputLane != -1) {
        bool hitRecorded = false;
        for (auto& pNote : s_PlayableNotes) {
            if (pNote.active && pNote.lane == inputLane) {
                float timeDiff = s_SongTimer - pNote.timeSec; 
                float absDiff = fabsf(timeDiff);

                if (absDiff <= 0.15f) { 
                    pNote.active = false;
                    float nX = LANE_X_COORDS[pNote.lane];
                    HitEffect::Spawn({nX, judgmentLineY});

                    s_ShowJudgment = true;
                    s_JudgmentTimer = 0.4f;
                    s_JudgmentLinePulse = 1.0f;
                    s_JudgmentAnimTimer = 0.3f;

                    if (absDiff <= 0.05f) {
                        s_CurrentJudgment = "PERFECT";
                        s_Combo++;
                    } else if (absDiff <= 0.10f) {
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
        s_ChartEditor.Render();
        return;
    }

    bool isDPressed = IsKeyDown(KEY_D);
    bool isFPressed = IsKeyDown(KEY_F);
    bool isJPressed = IsKeyDown(KEY_J);
    bool isKPressed = IsKeyDown(KEY_K);
    bool pressedStates[4] = { isDPressed, isFPressed, isJPressed, isKPressed };

    DrawBackground();
    DrawPlayfield();
    DrawUpperTechnicalHUD();
    DrawLanes(pressedStates, judgmentLineY);
    DrawInputFeedbackFlash(pressedStates, judgmentLineY);
    DrawNotes(judgmentLineY);
    DrawJudgmentLine(judgmentLineY);
    DrawComboHUD();
    DrawJudgmentText();
    DrawPlaySceneSideMarkers();
    DrawInputPanel(pressedStates);
    HitEffect::Draw();
}

void PlayScene::Unload() {
    if (s_MusicPlayer) {
        s_MusicPlayer->Stop();
    }

    if (s_ComboFont.texture.id != 0) UnloadFont(s_ComboFont);
    if (s_SuitFont.texture.id != 0) UnloadFont(s_SuitFont);
    s_ChartEditor.Release();
}