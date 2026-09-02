#include "play_scene.h"
#include "note.h"
#include "../editing/editor_scene.h"
#include <cmath>
#include <vector>

#ifdef _WIN32
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent(void);
#endif

extern "C" {
    extern const unsigned int new_piskel_data[1600]; 
    #ifndef NEW_PISKEL_FRAME_WIDTH
    #define NEW_PISKEL_FRAME_WIDTH 40
    #define NEW_PISKEL_FRAME_HEIGHT 40
    #endif
}

static std::vector<Note> s_Notes;
static Texture2D s_CustomNoteTex = { 0 };
static Texture2D s_ClickImageTex = { 0 };

static float s_SpawnTimer = 0.0f;
static bool  s_ShowPerfect = false;
static float s_PerfectTimer = 0.0f;

static const float LANE_Y_COORDS[4] = { 280.0f, 410.0f, 510.0f, 625.0f };

static bool  s_IsEditorMode = false;
static EditorScene s_EditorScene;

static float s_ClickImgX = 100.0f;
static float s_ClickImgY = 190.0f;
static const float MIN_Y_LIMIT = 50.0f;
static const float MAX_Y_LIMIT = 500.0f;
static bool  s_IsDragging = false;
static float s_DragOffsetY = 0.0f;

PlayScene::PlayScene() {
    arrowTex = { 0 };
    noteLineTex = { 0 };
    s_ClickImageTex = { 0 };
    judgmentLineX = 100.0f;
}

PlayScene::~PlayScene() {
}

void PlayScene::Init() {
    arrowTex = LoadTexture("assets/arrow.png");
    noteLineTex = LoadTexture("assets/noteLine.png");
    s_ClickImageTex = LoadTexture("assets/clickimage.png");
    judgmentLineX = 100.0f;

    Image rawImg = {
        .data = (void*)new_piskel_data,
        .width = NEW_PISKEL_FRAME_WIDTH,
        .height = NEW_PISKEL_FRAME_HEIGHT,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    Image scaledImg = ImageCopy(rawImg);
    ImageResize(&scaledImg, NEW_PISKEL_FRAME_WIDTH * 4, NEW_PISKEL_FRAME_HEIGHT * 4);

    s_CustomNoteTex = LoadTextureFromImage(scaledImg);
    UnloadImage(scaledImg);

    s_Notes.clear();
    s_SpawnTimer = 0.0f;
    s_ShowPerfect = false;
    s_IsEditorMode = false;
    s_IsDragging = false;
    
    s_EditorScene.Init();
}

void PlayScene::Update() {
    bool isDebugMode = false;
#ifdef _WIN32
    if (IsDebuggerPresent()) {
        isDebugMode = true;
    }
#endif

    if (isDebugMode && IsKeyPressed(KEY_P)) {
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

    float drawW = (s_ClickImageTex.id > 0) ? ((float)s_ClickImageTex.width / 2.0f) : 25.0f;
    float drawH = (s_ClickImageTex.id > 0) ? ((float)s_ClickImageTex.height / 2.0f) : 50.0f;

    Vector2 mousePos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        bool isHovered = (mousePos.x >= s_ClickImgX && mousePos.x <= s_ClickImgX + drawW &&
                          mousePos.y >= s_ClickImgY && mousePos.y <= s_ClickImgY + drawH);
        if (isHovered) {
            s_IsDragging = true;
            s_DragOffsetY = mousePos.y - s_ClickImgY;
        }
    }

    if (s_IsDragging) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            s_ClickImgY = mousePos.y - s_DragOffsetY;
            if (s_ClickImgY < MIN_Y_LIMIT) s_ClickImgY = MIN_Y_LIMIT;
            if (s_ClickImgY > MAX_Y_LIMIT) s_ClickImgY = MAX_Y_LIMIT;
        } else {
            s_IsDragging = false;
        }
    }

    s_SpawnTimer += dt;
    if (s_SpawnTimer >= 0.5f) {
        int lane = GetRandomValue(0, 3);
        s_Notes.push_back(Note(1300.0f, LANE_Y_COORDS[lane], 5.0f));
        s_SpawnTimer = 0.0f;
    }

    for (auto& note : s_Notes) {
        note.Update();
    }

    float hitboxCenterX = s_ClickImgX + (drawW / 2.0f);
    float hitboxCenterY = s_ClickImgY + (drawH / 2.0f);

    float hitboxWidth  = 60.0f;
    float hitboxHeight = 60.0f;

    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        for (auto& note : s_Notes) {
            if (note.active && 
                fabsf(note.x - hitboxCenterX) < (hitboxWidth / 2.0f) && 
                fabsf(note.y - hitboxCenterY) < (hitboxHeight / 2.0f)) {
                
                note.active = false;
                s_ShowPerfect = true;
                s_PerfectTimer = 0.5f;
                break;
            }
        }
    }

    if (s_ShowPerfect) {
        s_PerfectTimer -= dt;
        if (s_PerfectTimer <= 0.0f) {
            s_ShowPerfect = false;
        }
    }
}

void PlayScene::Draw() {
    if (s_IsEditorMode) {
        s_EditorScene.Render();
        return;
    }

    if (noteLineTex.id > 0) {
        Rectangle src = { 0.0f, 0.0f, (float)noteLineTex.width, (float)noteLineTex.height };
        Rectangle dst = { 0.0f, 100.0f, (float)GetScreenWidth(), (float)noteLineTex.height * 0.8f };
        DrawTexturePro(noteLineTex, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    if (arrowTex.id > 0) {
        float scale = 0.8f;
        float w = (float)arrowTex.width * scale;
        float h = (float)arrowTex.height * scale;
        Rectangle src = { 0.0f, 0.0f, (float)arrowTex.width, (float)arrowTex.height };
        Rectangle dst = { -120.0f, 190.0f, w, h };
        DrawTexturePro(arrowTex, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    if (s_ClickImageTex.id > 0) {
        Rectangle src = { 0.0f, 0.0f, (float)s_ClickImageTex.width, (float)s_ClickImageTex.height };
        float dw = (float)s_ClickImageTex.width / 2.0f;
        float dh = (float)s_ClickImageTex.height / 2.0f;
        Rectangle dst = { s_ClickImgX, s_ClickImgY, dw, dh };
        DrawTexturePro(s_ClickImageTex, src, dst, { 0.0f, 0.0f }, 0.0f, WHITE);
    }

    for (auto& note : s_Notes) {
        note.Draw(s_CustomNoteTex);
    }

    if (s_ShowPerfect) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        DrawText("PERFECT!", sw / 2 - 80, sh / 2 - 20, 40, YELLOW);
    }
}

void PlayScene::Unload() {
    if (arrowTex.id > 0) {
        UnloadTexture(arrowTex);
        arrowTex.id = 0;
    }
    if (noteLineTex.id > 0) {
        UnloadTexture(noteLineTex);
        noteLineTex.id = 0;
    }
    if (s_ClickImageTex.id > 0) {
        UnloadTexture(s_ClickImageTex);
        s_ClickImageTex.id = 0;
    }
    if (s_CustomNoteTex.id > 0) {
        UnloadTexture(s_CustomNoteTex);
        s_CustomNoteTex.id = 0;
    }
    
    s_EditorScene.Release();
}