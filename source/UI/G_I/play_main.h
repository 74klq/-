#ifndef PLAY_MAIN_H
#define PLAY_MAIN_H

#include "raylib.h"
#include <string>

struct SongInformation
{
    std::string title;
    std::string artist;
    std::string jacketPath;
    float bpm;
    float playTimeSec;
    std::string difficultyName;
    int difficultyLevel;
    int score;
    float accuracy;
    int maxCombo;
    int currentCombo;
    int perfectCount;
    int greatCount;
    int goodCount;
    int missCount;
    float hpRatio;
    float totalTimeSec;
};

class PlayMainUI
{
public:
    PlayMainUI();
    ~PlayMainUI();

    void Init();
    void Update(const SongInformation& songInfo);
    void Draw(const SongInformation& songInfo, int screenWidth, int screenHeight);

private:
    Font m_MainFont;
    bool m_FontLoaded;
    Texture2D m_JacketTexture;
    std::string m_LoadedJacketPath;
    float m_HealthPulseTimer;

    void DrawSidebarPanel(float x, float y, float width, float height);
    void DrawJacket(const SongInformation& songInfo, float x, float y, float size);
    void DrawSongInfo(const SongInformation& songInfo, float x, float y, float width);
    void DrawScoreAndAccuracy(const SongInformation& songInfo, float x, float y, float width);
    void DrawJudgements(const SongInformation& songInfo, float x, float y, float width);
    void DrawHealthGauge(const SongInformation& songInfo, float x, float y, float width, float height);
    void UnloadJacket();
};

#endif