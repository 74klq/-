#pragma once
#include <vector>
#include "raylib.h"
#include "chart_save.h"

class EditorPlay {
public:
    EditorPlay();
    ~EditorPlay();

    void Init(const std::vector<SaveNoteData>& notes, Texture2D noteTex);
    void Update(bool& isPlaying);
    void Draw();
    void Unload();

private:
    std::vector<SaveNoteData> playNotes;
    Texture2D tileTexture;

    Texture2D arrowTex;
    Texture2D noteLineTex;
    Texture2D clickImageTex;
    Texture2D customNoteTex;

    float clickImgX;
    float clickImgY;
    bool isDragging;
    float dragOffsetY;

    bool showPerfect;
    float perfectTimer;
};