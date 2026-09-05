#ifndef CHART_EDITOR_H
#define CHART_EDITOR_H

#include "raylib.h"
#include <vector>

class AudioManager;

namespace MusicExecute {
    class MusicPlayer1;
}

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
    
    // 💡 [최종 수정] 메모리 꼬임을 완벽히 차단하기 위해 둘 다 포인터(*) 구조로 관리합니다.
    MusicExecute::MusicPlayer1* m_MusicPlayer; 
    AudioManager* m_AudioManager; 
};

#endif
