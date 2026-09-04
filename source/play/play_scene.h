#ifndef PLAY_SCENE_H
#define PLAY_SCENE_H

#include "raylib.h"

class PlayScene {
private:
    float judgmentLineY;

public:
    PlayScene();
    ~PlayScene();
    void Init();
    void Update();
    void Draw();
    void Unload();
};

#endif