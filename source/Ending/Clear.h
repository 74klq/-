#ifndef CLEAR_H
#define CLEAR_H

#include "raylib.h"
#include <string>
#include <set>
#include <vector>

struct ClearResultData {
    std::string title;
    std::string artist;
    std::string difficultyName;
    int difficultyLevel;
    int score;
    float accuracy;
    int maxCombo;
    int perfectCount;
    int greatCount;
    int goodCount;
    int missCount;
    int breakCount;
    int fastCount;
    int slowCount;
    std::string grade;
    std::string jacketPath;
};

class ClearScreen
{
private:
    Font m_MainFont;
    Texture2D m_JacketTexture;
    std::string m_LoadedJacketPath;
    float m_AnimTimer;

    void UnloadJacket();

public:
    ClearScreen();
    ~ClearScreen();

    void Init();
    void Update(float dt);
    void Draw(const ClearResultData& result, int screenWidth, int screenHeight);
    void Release();
};

#endif