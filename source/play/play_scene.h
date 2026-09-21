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
    SongSelect& GetSongSelect() { return m_State == PlaySceneState::SongSelect ? m_SongSelect : m_SongSelect; }
    PlayScene(SongSelect& sharedSongSelect);
    ~PlayScene();

    void Init(int startSongIndex = 0);
    void Update();
    void Draw();
    void Unload();

    bool ShouldGoBackToMenu() const { return m_BackToMenu; }
    void ResetBackToMenuFlag() { m_BackToMenu = false; }

private:
    PlaySceneState m_State;
    SongSelect& m_SongSelect;
    bool m_BackToMenu;

    float judgmentLineY;
    void UpdatePlaying();
    void DrawPlaying();
};

#endif
