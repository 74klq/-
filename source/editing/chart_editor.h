#ifndef CHART_EDITOR_H
#define CHART_EDITOR_H

#include "raylib.h"
#include <vector>

struct ChartNote {
    int lane;
    float posX;
};

class ChartEditor {
public:
    ChartEditor();
    ~ChartEditor();

    void Init();
    void HandleInput();
    void Render();
    void Release();

private:
    Texture2D noteLineTexture;
    Texture2D arrowTexture;
    Texture2D tileTexture;
    
    float scrollOffset;
    std::vector<ChartNote> notes;
};

#endif