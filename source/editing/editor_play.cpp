#include "editor_play.h"
#include <cmath>
#include <vector>

static std::vector<bool> s_NoteActiveStates;

EditorPlay::EditorPlay() {
    arrowTex = { 0 };
    noteLineTex = { 0 };
    clickImageTex = { 0 };
    customNoteTex = { 0 };
    clickImgX = 100.0f;
    clickImgY = 190.0f;
    isDragging = false;
    dragOffsetY = 0.0f;
    showPerfect = false;
    perfectTimer = 0.0f;
}

EditorPlay::~EditorPlay() {
    Unload();
}

void EditorPlay::Init(const std::vector<SaveNoteData>& notes, Texture2D noteTex) {
    playNotes = notes;
    s_NoteActiveStates.assign(notes.size(), true);
    
    arrowTex = LoadTexture("assets/arrow.png");
    noteLineTex = LoadTexture("assets/noteLine.png");
    clickImageTex = LoadTexture("assets/clickimage.png");
    
    // 외부 piskel 데이터 의존성을 제거하고 텍스처를 비워둡니다 (DrawRectangle로 대체)
    customNoteTex = { 0 };

    clickImgX = 100.0f;
    clickImgY = 190.0f;
    isDragging = false;
    showPerfect = false;
    perfectTimer = 0.0f;
}

void EditorPlay::Update(bool& isPlaying) {
    if (IsKeyPressed(KEY_U)) {
        isPlaying = false;
        return;
    }

    float dt = GetFrameTime();
    float drawW = (clickImageTex.id > 0) ? ((float)clickImageTex.width / 2.0f) : 25.0f;
    float drawH = (clickImageTex.id > 0) ? ((float)clickImageTex.height / 2.0f) : 50.0f;
    Vector2 mousePos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool isHovered = (mousePos.x >= clickImgX && mousePos.x <= clickImgX + drawW &&
                          mousePos.y >= clickImgY && mousePos.y <= clickImgY + drawH);
        if (isHovered) {
            isDragging = true;
            dragOffsetY = mousePos.y - clickImgY;
        }
    }

    if (isDragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            clickImgY = mousePos.y - dragOffsetY;
            if (clickImgY < 50.0f) clickImgY = 50.0f;
            if (clickImgY > 500.0f) clickImgY = 500.0f;
        } else {
            isDragging = false;
        }
    }

    float noteSpeed = 400.0f; 
    for (auto& note : playNotes) {
        note.posX -= noteSpeed * dt;
    }

    const float laneYCoords[4] = { 280.0f, 410.0f, 510.0f, 625.0f };
    float hitboxCenterX = clickImgX + (drawW / 2.0f);
    float hitboxCenterY = clickImgY + (drawH / 2.0f);
    float hitboxWidth = 60.0f;
    float hitboxHeight = 60.0f;

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        for (size_t i = 0; i < playNotes.size(); ++i) {
            if (playNotes[i].lane < 0 || playNotes[i].lane >= 4) continue;

            float noteY = laneYCoords[playNotes[i].lane];
            float noteWidth = 60.0f; // 도형 노트 너비 기준
            float noteCenterX = playNotes[i].posX + (noteWidth / 2.0f);
            float noteCenterY = noteY;

            if (i < s_NoteActiveStates.size() && s_NoteActiveStates[i] && 
                fabsf(noteCenterX - hitboxCenterX) < (hitboxWidth / 2.0f) && 
                fabsf(noteCenterY - hitboxCenterY) < (hitboxHeight / 2.0f)) {
                
                s_NoteActiveStates[i] = false;
                showPerfect = true;
                perfectTimer = 0.5f;
                break;
            }
        }
    }

    if (showPerfect) {
        perfectTimer -= dt;
        if (perfectTimer <= 0.0f) {
            showPerfect = false;
        }
    }
}

void EditorPlay::Draw() {
    ClearBackground(BLACK);

    if (noteLineTex.id > 0) {
        Rectangle src = { 0.0f, 0.0f, (float)noteLineTex.width, (float)noteLineTex.height };
        Rectangle dst = { 0.0f, 100.0f, (float)GetScreenWidth(), (float)noteLineTex.height * 0.8f };
        DrawTexturePro(noteLineTex, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    if (arrowTex.id > 0) {
        float scale = 0.8f;
        float w = (float)arrowTex.width * scale;
        float h = (float)arrowTex.height * scale;
        Rectangle sourceRec = { 0.0f, 0.0f, (float)arrowTex.width, (float)arrowTex.height };
        Rectangle destRec = { -120.0f, 190.0f, w, h };
        DrawTexturePro(arrowTex, sourceRec, destRec, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    if (clickImageTex.id > 0) {
        Rectangle src = { 0.0f, 0.0f, (float)clickImageTex.width, (float)clickImageTex.height };
        float dw = (float)clickImageTex.width / 2.0f;
        float dh = (float)clickImageTex.height / 2.0f;
        Rectangle dst = { clickImgX, clickImgY, dw, dh };
        DrawTexturePro(clickImageTex, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    const float laneYCoords[4] = { 280.0f, 410.0f, 510.0f, 625.0f };
    for (size_t i = 0; i < playNotes.size(); ++i) {
        if (playNotes[i].lane < 0 || playNotes[i].lane >= 4) continue;
        if (i < s_NoteActiveStates.size() && !s_NoteActiveStates[i]) continue;

        float noteY = laneYCoords[playNotes[i].lane] - 7.5f; // 높이 중앙 보정
        if (customNoteTex.id > 0) {
            DrawTexture(customNoteTex, (int)playNotes[i].posX, (int)noteY, WHITE);
        } else {
            // 텍스처 대신 하얀색 직사각형 노트로 렌더링
            DrawRectangle((int)playNotes[i].posX, (int)noteY, 60, 15, WHITE);
        }
    }

    if (showPerfect) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        DrawText("PERFECT!", sw / 2 - 80, sh / 2 - 20, 40, YELLOW);
    }
}

void EditorPlay::Unload() {
    if (arrowTex.id > 0) { UnloadTexture(arrowTex); arrowTex.id = 0; }
    if (noteLineTex.id > 0) { UnloadTexture(noteLineTex); noteLineTex.id = 0; }
    if (clickImageTex.id > 0) { UnloadTexture(clickImageTex); clickImageTex.id = 0; }
    if (customNoteTex.id > 0) { UnloadTexture(customNoteTex); customNoteTex.id = 0; }
    playNotes.clear();
    s_NoteActiveStates.clear();
}