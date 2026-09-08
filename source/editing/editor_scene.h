#pragma once
#include "raylib.h"

class EditorScene {
private:
    Texture2D noteLineTexture;
    Texture2D arrowTexture;
    float scrollOffset;        

public:
    EditorScene();
    ~EditorScene();

    void Init();
    void HandleInput();        
    void Render();
    void Release();

    float GetScrollOffset() const { return scrollOffset; }
};