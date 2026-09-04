#include "chart_editor.h"
#include "chart_save.h"
#include "editor_play.h"
#include <cmath>
#include <vector>

static EditorPlay s_EditorPlay;
static bool s_IsTestPlaying = false;

ChartEditor::ChartEditor() 
    : scrollOffset(0.0f) {
    noteLineTexture = { 0 };
    arrowTexture = { 0 };
    tileTexture = { 0 };
}

ChartEditor::~ChartEditor() {
    Release();
}

void ChartEditor::Init() {
    noteLineTexture = LoadTexture("assets/noteLine.png");
    arrowTexture = LoadTexture("assets/arrow.png");
    scrollOffset = 0.0f;
    s_IsTestPlaying = false;

    // 외부 piskel 데이터 의존성을 제거하고 텍스처를 비워둡니다 (렌더링 시 도형으로 대체)
    tileTexture = { 0 };
}

void ChartEditor::HandleInput() {
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
        s_EditorPlay.Init(saveData, tileTexture);
        return;
    }

    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0.0f) {
        scrollOffset -= wheelMove * 50.0f;
        if (scrollOffset < 0.0f) {
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

    int screenWidth = GetScreenWidth();
    const float laneYCoords[4] = { 267.0f, 390.0f, 510.0f, 625.0f };
    float tolerance = 25.0f; 

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mousePos = GetMousePosition();

        if (mousePos.x >= 0 && mousePos.x <= screenWidth) {
            int bestLane = -1;
            float minDistance = 9999.0f;

            for (int i = 0; i < 4; ++i) {
                float distance = fabsf(mousePos.y - laneYCoords[i]);
                if (distance < minDistance) {
                    minDistance = distance;
                    bestLane = i;
                }
            }

            if (bestLane != -1 && minDistance <= tolerance) {
                float worldX = mousePos.x + scrollOffset;
                notes.push_back({bestLane, worldX});
            }
        }
    }
}

void ChartEditor::Render() {
    if (s_IsTestPlaying) {
        s_EditorPlay.Draw();
        return;
    }

    int screenWidth = GetScreenWidth();

    float scale = 0.8f;
    float drawWidth = (float)noteLineTexture.width * scale;
    float drawHeight = (float)noteLineTexture.height * scale;
    
    float yPos = 100.0f; 
    float overlapPixels = 45.0f; 
    float effectiveWidth = drawWidth - overlapPixels;

    int startIndex = (int)(scrollOffset / effectiveWidth);
    int endIndex = startIndex + (int)(screenWidth / effectiveWidth) + 2;

    if (noteLineTexture.id > 0) {
        for (int i = endIndex; i >= startIndex; --i) {
            float drawX = (i * effectiveWidth) - scrollOffset;
            Rectangle sourceRec = { 0.0f, 0.0f, (float)noteLineTexture.width, (float)noteLineTexture.height };
            Rectangle destRec = { drawX, yPos, drawWidth, drawHeight };
            Vector2 origin = { 0.0f, 0.0f };
            DrawTexturePro(noteLineTexture, sourceRec, destRec, origin, 0.0f, WHITE);
        }
    }

    const float laneYCoords[4] = { 290.0f, 400.0f, 510.0f, 625.0f };

    for (const auto& note : notes) {
        float screenNoteX = note.posX - scrollOffset;
        if (screenNoteX >= -50 && screenNoteX <= screenWidth + 50) {
            float noteY = laneYCoords[note.lane] - 20.0f; // 높이 보정
            
            if (tileTexture.id > 0) {
                DrawTexture(tileTexture, screenNoteX, noteY, WHITE);
            } else {
                // 텍스처가 없을 경우 하얀색 가로형 직사각형 노트로 직접 렌더링
                DrawRectangle((int)screenNoteX, (int)noteY, 60, 15, WHITE);
            }
        }
    }

    if (arrowTexture.id > 0) {
        float arrowScale = 0.9f; 
        float arrowWidth = (float)arrowTexture.width * arrowScale;
        float arrowHeight = (float)arrowTexture.height * arrowScale;
        float arrowYPos = 150.0f; 
        float arrowX = -130.0f - scrollOffset;

        Rectangle sourceRec = { 0.0f, 0.0f, (float)arrowTexture.width, (float)arrowTexture.height };
        Rectangle destRec = { arrowX, arrowYPos, arrowWidth, arrowHeight };
        Vector2 origin = { 0.0f, 0.0f };
        DrawTexturePro(arrowTexture, sourceRec, destRec, origin, 0.0f, WHITE);
    }
}

void ChartEditor::Release() {
    if (noteLineTexture.id > 0) UnloadTexture(noteLineTexture);
    if (arrowTexture.id > 0) UnloadTexture(arrowTexture);
    if (tileTexture.id > 0) UnloadTexture(tileTexture);
}