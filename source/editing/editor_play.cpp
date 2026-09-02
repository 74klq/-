#include "editor_play.h"
#include <cmath>
#include <vector>

extern "C" {
    extern const unsigned int new_piskel_data[1600];
    #ifndef NEW_PISKEL_FRAME_WIDTH
    #define NEW_PISKEL_FRAME_WIDTH 40
    #define NEW_PISKEL_FRAME_HEIGHT 40
    #endif
}

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
    
    Image noteImg = {
        .data = (void*)new_piskel_data,
        .width = NEW_PISKEL_FRAME_WIDTH,
        .height = NEW_PISKEL_FRAME_HEIGHT,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    Image resizedImg = ImageCopy(noteImg);
    ImageResize(&resizedImg, NEW_PISKEL_FRAME_WIDTH * 4, NEW_PISKEL_FRAME_HEIGHT * 4);
    customNoteTex = LoadTextureFromImage(resizedImg);
    UnloadImage(resizedImg);

    clickImgX = 100.0f;
    clickImgY = 190.0f;
    isDragging = false;
    showPerfect = false;
    perfectTimer = 0.0f;
}

void EditorPlay::Update(bool& isPlaying) {
    // 테스트 플레이 종료 키를 U로 변경
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
            float noteWidth = (customNoteTex.id > 0) ? (float)customNoteTex.width : 30.0f;
            float noteCenterX = playNotes[i].posX + (noteWidth / 2.0f);
            float noteCenterY = noteY;

            // 벡터 인덱스 초과 방지 안전 검사 추가
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

        float noteY = laneYCoords[playNotes[i].lane] - ((float)customNoteTex.height / 2.0f);
        if (customNoteTex.id > 0) {
            DrawTexture(customNoteTex, (int)playNotes[i].posX, (int)noteY, WHITE);
        } else {
            DrawRectangle((int)playNotes[i].posX, (int)noteY, 30, 40, RED);
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