#ifndef PLAY_SCENE_H
#define PLAY_SCENE_H

#include "raylib.h"
#include "../Menu/Map_Selection/Selection.h"

enum class PlaySceneState {
    SongSelect,
    Playing
};

class PlayScene {
public:
    PlayScene();
    ~PlayScene();

    void Init();
    void Update();
    void Draw();
    void Unload();

    bool ShouldGoBackToMenu() const { return m_BackToMenu; }

private:
    PlaySceneState m_State;
    SongSelect m_SongSelect;
    bool m_BackToMenu;

    float judgmentLineY;
    void UpdatePlaying();
    void DrawPlaying();
};

#endif