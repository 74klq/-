#ifndef CHART_EDITOR_H
#define CHART_EDITOR_H

#include "raylib.h"
#include <vector>
#include "../music_execute/music1_on.cpp"
#include "../AudioManager/audio_manager.h"

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
    float scrollOffset;
    std::vector<ChartNote> notes;
    MusicExecute::MusicPlayer1 m_MusicPlayer;
    AudioManager m_AudioManager;
};

#endif