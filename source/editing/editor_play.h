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
};