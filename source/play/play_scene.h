#ifndef PLAY_SCENE_H
#define PLAY_SCENE_H

#include "raylib.h"

class PlayScene {
private:
    Texture2D arrowTex;
    Texture2D noteLineTex;
    float judgmentLineX;

public:
    PlayScene();
    ~PlayScene();
    void Init();
    void Update();
    void Draw();
    void Unload();
};

#endif