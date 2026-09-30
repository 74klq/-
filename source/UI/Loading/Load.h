#pragma once

#include "raylib.h"
#include <string>

enum class LoadingState {
    IDLE,
    CURTAIN_IN,
    LOADING,
    CURTAIN_OUT,
    COMPLETE
};

struct LoadingSongData {
    Texture2D jacketTexture;
    std::string title;
    std::string artist;
};

class LoadingScreen {
public:
    LoadingScreen();
    ~LoadingScreen();

    void Init();
    void Start(const LoadingSongData& songData);
    void Update(float dt);
    void Draw(); //(Font customFont);

    void SetTargetProgress(float progress);

    bool IsCurtainClosed() const;
    bool IsComplete() const;
    LoadingState GetState() const;

private:
    LoadingState currentState;
    LoadingSongData currentSong;

    float curtainProgress;
    float currentLoadProgress;
    float targetLoadProgress;
    
    float stateTimer;

    Font suitFont;
    bool fontLoaded;

    void DrawCurtain();
    void DrawJacket();
    void DrawProgressBar(); //(Font customFont);
    
    float EaseOutCubic(float x);
    float EaseInOutQuad(float x);
};